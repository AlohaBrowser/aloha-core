
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_URL_KEYWORD_EXTRACTOR_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_URL_KEYWORD_EXTRACTOR_H_

#include <string>
#include <string_view>

#include "absl/types/optional.h"
#include "url/gurl.h"

namespace adblock {

// Keywords allow selecting filters that could potentially match a URL faster
// than an exhaustive search.
// This is how it works:
// 1. Each URL is split into keywords using
// GetNextKeyword().
// "https://content.adblockplus.com/advertisment" becomes:
// "content", "adblockplus", "advertisment"
// - "https" and "com" are skipped because they're common
//
// 2. The keyword index in the flatbuffer is queried only for filters that match
// these keywords. A keyword may index multiple filters, a filter is only
// indexed by one (or none) keywords.
//
// 3. If we fail to extract keywords from a filter, we index it under an empty
// keyword. All filters without a keyword are checked for all URLs, as they
// could match anything.
class UrlKeywordExtractor {
 public:
  explicit UrlKeywordExtractor(std::string_view url);
  ~UrlKeywordExtractor();
  absl::optional<std::string_view> GetNextKeyword();

 private:
  std::string url_with_nulls_;
  std::string_view input_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_URL_KEYWORD_EXTRACTOR_H_
