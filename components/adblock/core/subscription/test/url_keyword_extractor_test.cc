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

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

TEST(AdblockUrlKeywordExtractor, NoKeywordExtractedFromEmptyInput_Request) {
  UrlKeywordExtractor extractor("");
  EXPECT_EQ(extractor.GetNextKeyword(), absl::nullopt);
}

TEST(AdblockUrlKeywordExtractor, DoesNotExtractCommonKeywords) {
  // Common keywords include "http", "https", "com" and "js". These should be
  // skipped.
  UrlKeywordExtractor extractor("http://www.base.com/path?query.js");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords,
              testing::ElementsAre("www", "base", "path", "query"));
}

TEST(AdblockUrlKeywordExtractor, DoesExtractLastKeywordsForRequest) {
  UrlKeywordExtractor extractor("http://domain.cc/in_discovery5");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords,
              testing::ElementsAre("domain", "cc", "in", "discovery5"));
}

TEST(AdblockUrlKeywordExtractor, SingleLetterKeywordsSkipped) {
  UrlKeywordExtractor extractor("http://a.b/cc");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords, testing::ElementsAre("cc"));
}

TEST(AdblockUrlKeywordExtractor, KeywordSymbolVsSeparatorSymbols) {
  // Keyword symbols are alphanumeric characters plus the % symbol. Everything
  // else is a separator.
  UrlKeywordExtractor extractor("http://alpha.beta/data123-data2%4?");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords,
              testing::ElementsAre("alpha", "beta", "data123", "data2%4"));
}

}  // namespace adblock
