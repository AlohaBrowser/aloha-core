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

#include "components/adblock/core/subscription/url_keyword_extractor.h"

#include <algorithm>
#include <cctype>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/common/keyword_extractor_utils.h"

namespace adblock {
namespace {

bool IsSeparatorCharacter(char c) {
  return !(std::isalnum(c) || c == '%');
}

}  // namespace

absl::optional<std::string_view> UrlKeywordExtractor::GetNextKeyword() {
  std::string_view current_keyword;
  do {
    const auto start_of_next_keyword = input_.find_first_not_of('\0');
    if (start_of_next_keyword == std::string_view::npos) {
      return absl::nullopt;
    }
    input_.remove_prefix(start_of_next_keyword);
    const auto end_of_keyword = input_.find_first_of('\0');
    current_keyword = input_.substr(0, end_of_keyword);
    input_.remove_prefix(current_keyword.size());
  } while (utils::IsBadKeyword(current_keyword));
  return current_keyword;
}

UrlKeywordExtractor::UrlKeywordExtractor(std::string_view url)
    : url_with_nulls_(url.data(), url.size()) {
  // The keywords returned by GetNextKeyword() will be passed to
  // flatbuffers::Vector::LookupByKey(const char* key) which assumes |key| is
  // null-terminated. In order to avoid allocating a null-terminated
  // std::string for every extracted keyword, we instead replace separator
  // characters by nulls, so that a StringPiece referring to a keyword is also
  // null-terminated.
  // This isn't elegant, but it's a valid workaround for the limitations of
  // the flatbuffers generated API.
  base::ranges::replace_if(url_with_nulls_, &IsSeparatorCharacter, '\0');
  input_ = url_with_nulls_;
}

UrlKeywordExtractor::~UrlKeywordExtractor() = default;

}  // namespace adblock
