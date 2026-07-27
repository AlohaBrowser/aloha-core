/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/eyeo_testing/libadblockplus_reference_database.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "components/eyeo_testing/test_request_reader.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "sql/test/test_helpers.h"
#include "sql/transaction.h"

namespace adblock {
namespace test {
namespace {

constexpr int kSubdocumentContentType = 32;
enum ColumnIndex {
  Id = 0,
  Url,
  ContentType,
  Domain,
  ShouldBlock,
  ElemhideCssHash,
  ElemhideEmuSelectorsHash,
  SnippetsHash,
};

int64_t ComputeSelectorsHash(std::vector<std::string> selectors) {
  if (selectors.empty()) {
    return -1;  // For easier troubleshooting, as empty inputs are common
  }
  std::ranges::sort(selectors);
  return base::PersistentHash(base::JoinString(selectors, " "));
}
int64_t ComputeSnippetsHash(const std::string& snippets) {
  if (snippets.empty()) {
    return -1;
  }
  return base::PersistentHash(snippets);
}

int64_t ComputeStylesheetHash(const std::string& css) {
  if (css.empty()) {
    return -1;
  }
  // The generated css is composed out of multiple selectors. They can be
  // returned in any order and there can be repetitions. In order to compare,
  // sort them and make them unique before joining them again for hashing.
  auto split = base::SplitString(css, ", \n", base::KEEP_WHITESPACE,
                                 base::SPLIT_WANT_ALL);
  // This is all pretty slow, but that's the APIs we're dealing with.
  std::ranges::sort(split);
  split.erase(std::unique(split.begin(), split.end()), split.end());
  return base::PersistentHash(base::JoinString(split, " "));
}

}  // namespace

LibadblockplusReferenceDatabase::ElemhideResults::ElemhideResults() = default;
LibadblockplusReferenceDatabase::ElemhideResults::ElemhideResults(
    std::string elemhide_css,
    std::vector<std::string> elemhide_emu_selectors,
    std::string snippets)
    : elemhide_css(std::move(elemhide_css)),
      elemhide_emu_selectors(std::move(elemhide_emu_selectors)),
      snippets(std::move(snippets)) {}
LibadblockplusReferenceDatabase::ElemhideResults::ElemhideResults(
    const ElemhideResults&) = default;
LibadblockplusReferenceDatabase::ElemhideResults&
LibadblockplusReferenceDatabase::ElemhideResults::operator=(
    const ElemhideResults&) = default;
LibadblockplusReferenceDatabase::ElemhideResults::~ElemhideResults() = default;

LibadblockplusReferenceDatabase::LibadblockplusReferenceDatabase(
    const base::FilePath& db_path)
    : db_(sql::DatabaseOptions(), sql::test::kTestTag) {
  CHECK(db_.Open(db_path));
}
void LibadblockplusReferenceDatabase::StoreInputsFromFile(
    const base::FilePath& request_data_file,
    FilterMatchCallback reference_match_implementation,
    ElemhideCallback reference_elemhide_implementation) {
  RecreateDatabase();
  TestRequestReader request_reader;
  request_reader.Open(request_data_file);
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    LOG(ERROR) << "sql::Transaction failed!";
    return;
  }
  while (auto request = request_reader.GetNextRequest()) {
    if (request->content_type > 32768) {
      // This isn't a real content type, it's a *filter type*. Old core would
      // conflate the two. Ignore it.
      continue;
    }
    if ((request->line_number % 10000) == 0) {
      LOG(INFO) << "Processed lines: " << request->line_number;
    }
    const bool should_block = reference_match_implementation.Run(*request);
    sql::Statement request_insert(db_.GetCachedStatement(
        SQL_FROM_HERE, "INSERT INTO request VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    request_insert.BindInt(ColumnIndex::Id, request->line_number);
    request_insert.BindString(ColumnIndex::Url, request->url.spec());
    request_insert.BindInt64(ColumnIndex::ContentType, request->content_type);
    request_insert.BindString(ColumnIndex::Domain, request->domain);
    request_insert.BindInt(ColumnIndex::ShouldBlock, should_block);
    if (request->content_type == kSubdocumentContentType) {
      const ElemhideResults results =
          reference_elemhide_implementation.Run(*request);
      request_insert.BindInt64(ColumnIndex::ElemhideCssHash,
                               ComputeStylesheetHash(results.elemhide_css));
      request_insert.BindInt64(
          ColumnIndex::ElemhideEmuSelectorsHash,
          ComputeSelectorsHash(results.elemhide_emu_selectors));
      request_insert.BindInt64(ColumnIndex::SnippetsHash,
                               ComputeSnippetsHash(results.snippets));
    } else {
      request_insert.BindNull(ColumnIndex::ElemhideCssHash);
      request_insert.BindNull(ColumnIndex::ElemhideEmuSelectorsHash);
      request_insert.BindNull(ColumnIndex::SnippetsHash);
    }
    CHECK(request_insert.Run());
  }
  CHECK(transaction.Commit());
}

void LibadblockplusReferenceDatabase::CompareImplementationAgainstReference(
    FilterMatchCallback tested_match_implementation,
    ElemhideCallback tested_elemhide_implementation,
    MismatchCallback mismatch_callback,
    int task_id,
    int task_count) {
  sql::Statement select_request(db_.GetCachedStatement(
      SQL_FROM_HERE, "SELECT * FROM request WHERE id % ? = ?"));
  select_request.BindInt(0, task_count);
  select_request.BindInt(1, task_id);
  CompareWithStatement(select_request, tested_match_implementation,
                       tested_elemhide_implementation, mismatch_callback);
}

void LibadblockplusReferenceDatabase::
    CompareImplementationAgainstSpecificRequests(
        FilterMatchCallback tested_match_implementation,
        ElemhideCallback tested_elemhide_implementation,
        MismatchCallback mismatch_callback,
        const std::string& request_ids) {
  sql::Statement select_request(db_.GetCachedStatement(
      SQL_FROM_HERE, "SELECT * FROM request WHERE id IN (?)"));
  select_request.BindString(0, request_ids);
  CompareWithStatement(select_request, tested_match_implementation,
                       tested_elemhide_implementation, mismatch_callback);
}

void LibadblockplusReferenceDatabase::CompareWithStatement(
    sql::Statement& select_request,
    FilterMatchCallback tested_match_implementation,
    ElemhideCallback tested_elemhide_implementation,
    MismatchCallback mismatch_callback) {
  while (select_request.Step()) {
    Request request;
    request.line_number = select_request.ColumnInt(ColumnIndex::Id);
    request.url = GURL(select_request.ColumnString(ColumnIndex::Url));
    request.content_type = select_request.ColumnInt64(ColumnIndex::ContentType);
    request.domain = select_request.ColumnString(ColumnIndex::Domain);
    if (!request.url.is_valid() ||
        !GURL("https://" + request.domain).is_valid()) {
      // Invalid URLs might exist in the dataset, it was created long ago
      // with different validation rules. For example, the dataset contains
      // https://series%20latinoamerica.disqus.com/embed.js, which used to
      // be a valid GURL but a space in the domain is no longer allowed.
      continue;
    }
    const bool reference_blocks =
        select_request.ColumnBool(ColumnIndex::ShouldBlock);
    // If the tested implementation returns a different result than reference,
    // notify the mismatch callback.
    const bool tested_blocks = tested_match_implementation.Run(request);
    if (tested_blocks != reference_blocks) {
      mismatch_callback.Run(request.line_number, MismatchType::UrlFilterMatch);
    }
    if (request.content_type == kSubdocumentContentType) {
      // In addition to filter matching, we should have element hiding data
      // for this request.
      // If hashed elemhide results are different from reference, notify
      // mismatch callback.
      const ElemhideResults tested_results =
          tested_elemhide_implementation.Run(request);
      if (ComputeStylesheetHash(tested_results.elemhide_css) !=
          select_request.ColumnInt64(ColumnIndex::ElemhideCssHash)) {
        mismatch_callback.Run(request.line_number, MismatchType::ElemhideCss);
      }
      if (ComputeSelectorsHash(tested_results.elemhide_emu_selectors) !=
          select_request.ColumnInt64(ColumnIndex::ElemhideEmuSelectorsHash)) {
        mismatch_callback.Run(request.line_number,
                              MismatchType::ElemhideEmuSelectors);
      }
// TODO DPD-1279: re-enable comparison of snippets.
#if 0
      if (ComputeSnippetsHash(tested_results.snippets) !=
          select_request.ColumnInt64(ColumnIndex::SnippetsHash)) {
        mismatch_callback.Run(request.line_number, MismatchType::Snippets);
      }
#endif  // 0
    }
    if (request.line_number % 10000 == 0) {
      LOG(INFO) << "Processed " << request.line_number << " requests";
    }
  }
}

void LibadblockplusReferenceDatabase::RecreateDatabase() {
  CHECK(db_.Raze());
  CHECK(db_.Execute(R"(
    CREATE TABLE request (
      id INTEGER PRIMARY KEY,
      url TEXT NOT NULL,
      content_type INTEGER NOT NULL,
      domain TEXT NOT NULL,
      should_block INTEGER NOT NULL,
      elemhide_css_hash INTEGER,
      elemhide_emu_selectors_hash INTEGER,
      snippets_hash INTEGER
    )
    )"));
}

}  // namespace test
}  // namespace adblock
