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

#include "components/adblock/core/converter/serializer/filter_keyword_extractor.h"

#include <algorithm>
#include <cctype>

#include "base/strings/string_util.h"
#include "components/adblock/core/common/keyword_extractor_utils.h"
#include "third_party/re2/src/re2/re2.h"

namespace adblock {

absl::optional<std::string> FilterKeywordExtractor::GetNextKeyword() {
  std::string current_keyword;
  do {
    // In case that we are extracting keyword to store a filter
    // we need to be careful as only one keyword will be used.
    // So a keyword at the end of the filter might mismatch with keyword
    // when trying to fetch the filter
    // for example:
    // a filter ||domain.cc/in_discovery should not retrieve "discovery" as a
    // keyword because when we have a valid to block url like this one
    // domain.cc/in_discovery5 returns with "discovery5" as
    // one of the extracted keywords instead of "discovery"
    static const re2::RE2 filter_keyword_extractor(
        "([^a-zA-Z0-9%*][a-zA-Z0-9%]{2,})");
    static const re2::RE2 has_a_following_keyword("(^[^a-zA-Z0-9%*])");
    static const re2::RE2 following_keyword_consume("(^[a-zA-Z0-9%*]*)");
    if (!RE2::FindAndConsume(&input_, filter_keyword_extractor,
                             &current_keyword)) {
      return absl::nullopt;
    }
    if (!RE2::PartialMatch(input_, has_a_following_keyword)) {
      RE2::Consume(&input_, following_keyword_consume);
      current_keyword.clear();
      continue;
    }
    current_keyword = current_keyword.substr(1);
  } while (utils::IsBadKeyword(current_keyword));
  return base::ToLowerASCII(current_keyword);
}

FilterKeywordExtractor::FilterKeywordExtractor(std::string_view url)
    : input_(url.data(), url.size()), end_of_last_keyword_(input_.begin()) {}
FilterKeywordExtractor::~FilterKeywordExtractor() = default;

}  // namespace adblock
