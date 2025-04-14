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

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

TEST(AdblockFilterKeywordExtractor, NoKeywordExtractedFromEmptyInput) {
  FilterKeywordExtractor extractor("");
  EXPECT_EQ(extractor.GetNextKeyword(), absl::nullopt);
}

TEST(AdblockFilterKeywordExtractor, DoesNotExtractCommonKeywords) {
  FilterKeywordExtractor extractor("||https://domain.com/script.js");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords, testing::ElementsAre("domain", "script"));
}

TEST(AdblockFilterKeywordExtractor, SingleLetterKeywordsSkipped) {
  FilterKeywordExtractor extractor("||a.com");
  EXPECT_EQ(extractor.GetNextKeyword(), absl::nullopt);
}

TEST(AdblockFilterKeywordExtractor, DoesNotExtractLastKeywords) {
  FilterKeywordExtractor extractor("||domain.cc/in_discovery");
  // This filter should match "http://domain.cc/in_discovery5". Because the
  // Converter only stores the longest keyword per filter, we don't want the
  // trailing "discovery" component to "win", as it would not match longer
  // keywords extracted from requests. We skip the trailing keyword when
  // extracting for filter, but we include it when extracting from request.
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords, testing::ElementsAre("domain", "cc", "in"));
}

TEST(AdblockKeywordExtractor, DoesNotExtractWildcardKeyword) {
  FilterKeywordExtractor extractor("/path1/test*iles/path2/file.js");
  std::vector<std::string> extracted_keywords;
  while (auto keyword = extractor.GetNextKeyword()) {
    extracted_keywords.push_back(keyword->data());
  }
  EXPECT_THAT(extracted_keywords,
              testing::ElementsAre("path1", "path2", "file"));
}

}  // namespace adblock
