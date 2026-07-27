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

#include "components/adblock/core/snippet/snippet_service_impl.h"

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace adblock {

SnippetUpdateServiceImpl::SnippetUpdateServiceImpl(
    std::unique_ptr<SnippetStorage> storage,
    std::unique_ptr<SnippetDownloader> downloader)
    : storage_(std::move(storage)), downloader_(std::move(downloader)) {
  downloader_->StartSchedule();
}

SnippetUpdateServiceImpl::~SnippetUpdateServiceImpl() = default;

std::string SnippetUpdateServiceImpl::GenerateSnippetScript(
    const GURL& url,
    base::ListValue input) {
  TRACE_EVENT1("eyeo", "GenerateSnippetScript", "url", url.spec());
  // snippets must be JSON representation of the array of arrays of snippets
  std::string serialized;
  JSONStringValueSerializer serializer(&serialized);
  serializer.Serialize(std::move(input));
  std::string output = "{{xpath3}}({{callback}})({}, ...{{snippets}});";
  bool require_xpath3 =
      serialized.find("hide-if-matches-xpath3") != std::string::npos;
  base::ReplaceSubstringsAfterOffset(&output, 0, "{{xpath3}}",
                                     require_xpath3 ? GenerateXpath3Dep() : "");
  base::ReplaceSubstringsAfterOffset(&output, 0, "{{callback}}",
                                     storage_->GetLoadedSnippets());
  base::ReplaceSubstringsAfterOffset(&output, 0, "{{snippets}}", serialized);
  return output;
}

std::string SnippetUpdateServiceImpl::GenerateXpath3Dep() {
  if (xpath3_dep_.empty()) {
    xpath3_dep_ =
        "(" +
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            IDR_ADBLOCK_SNIPPETS_XPATH3_DEP_JS) +
        ")();";
  }
  if (xpath3_dep_ == "()();") {
    LOG(WARNING) << "[eyeo] Snippet library does not support xpath3!";
    return "";
  }
  return xpath3_dep_;
}

std::string SnippetUpdateServiceImpl::GetSnippetVersion() {
  auto& library = storage_->GetLoadedSnippets();
  std::string needle = "// ! Version: ";
  auto start = library.find_first_of(needle);
  auto end = library.find_first_of('\n');
  if (start != std::string::npos && end != std::string::npos &&
      start + needle.length() < end) {
    return library.substr(start + needle.length(),
                          end - (start + needle.length()));
  }
  return "";
}

}  // namespace adblock
