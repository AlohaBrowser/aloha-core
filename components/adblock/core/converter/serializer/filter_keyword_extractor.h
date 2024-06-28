
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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FILTER_KEYWORD_EXTRACTOR_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FILTER_KEYWORD_EXTRACTOR_H_

#include <string>
#include <string_view>

#include "absl/types/optional.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace adblock {

// Keywords allow selecting filters that could potentially match a URL faster
// than an exhaustive search.
// This is how it works:
//
// 1. A filter pattern is split into keywords via GetNextKeyword()
// like so:
// ||content.adblockplus.com/ad
// becomes:
// "content", "adblockplus"
// - "com" is skipped because it's a very common component
// - "ad" is skipped, explanation in .cc
//
// 2. Once we have keywords that describe the filter, the longest or most unique
// keyword gets chosen to index the filter within the flatbuffer. In this case,
// "adblockplus".
class FilterKeywordExtractor {
 public:
  explicit FilterKeywordExtractor(std::string_view url);
  ~FilterKeywordExtractor();
  absl::optional<std::string> GetNextKeyword();

 private:
  re2::StringPiece input_;
  re2::StringPiece::iterator end_of_last_keyword_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FILTER_KEYWORD_EXTRACTOR_H_
