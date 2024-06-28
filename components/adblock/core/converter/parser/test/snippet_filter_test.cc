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

#include "components/adblock/core/converter/parser/snippet_filter.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using ::testing::ElementsAre;

TEST(AdblockSnippetFilterTest, ParseEmptySnippetFilter) {
  EXPECT_FALSE(SnippetFilter::FromString("", "").has_value());
}

TEST(AdblockSnippetFilterTest, ParseSnippetFilterEmptySnippet) {
  EXPECT_FALSE(SnippetFilter::FromString("test.com", "").has_value());
}

TEST(AdblockSnippetFilterTest, ParseSnippetFilter) {
  auto snippet_filter = SnippetFilter::FromString("test.com", "snippet");
  ASSERT_TRUE(snippet_filter.has_value());
  ASSERT_EQ(snippet_filter->snippet_script.size(), 1u);
  ASSERT_EQ(snippet_filter->snippet_script.at(0).size(), 1u);
  EXPECT_EQ(snippet_filter->snippet_script.at(0).at(0), "snippet");
  EXPECT_THAT(snippet_filter->domains.GetIncludeDomains(),
              ElementsAre("test.com"));
  EXPECT_TRUE(snippet_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockSnippetFilterTest, ParseSnippetFilterSnippetGetsTokenized) {
  auto snippet_filter =
      SnippetFilter::FromString("test.com", "snippet log Test");
  ASSERT_TRUE(snippet_filter.has_value());
  ASSERT_EQ(snippet_filter->snippet_script.size(), 1u);
  ASSERT_EQ(snippet_filter->snippet_script.at(0).size(), 3u);
  EXPECT_EQ(snippet_filter->snippet_script.at(0).at(0), "snippet");
  EXPECT_EQ(snippet_filter->snippet_script.at(0).at(1), "log");
  EXPECT_EQ(snippet_filter->snippet_script.at(0).at(2), "Test");
  EXPECT_THAT(snippet_filter->domains.GetIncludeDomains(),
              ElementsAre("test.com"));
  EXPECT_TRUE(snippet_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockSnippetFilterTest, ParseSnippetFilterNoIncludeDomain) {
  EXPECT_FALSE(SnippetFilter::FromString("~test.com", "snippet").has_value());
}

TEST(AdblockSnippetFilterTest,
     ParseSnippetFilterDomainsWithNoSubdomainRemoved) {
  auto snippet_filter = SnippetFilter::FromString(
      "org,example.org,~localhost,~example_too.org", "snippet");
  ASSERT_TRUE(snippet_filter.has_value());
  ASSERT_EQ(snippet_filter->snippet_script.size(), 1u);
  ASSERT_EQ(snippet_filter->snippet_script.at(0).size(), 1u);
  EXPECT_EQ(snippet_filter->snippet_script.at(0).at(0), "snippet");
  EXPECT_THAT(snippet_filter->domains.GetIncludeDomains(),
              ElementsAre("example.org"));
  EXPECT_THAT(snippet_filter->domains.GetExcludeDomains(),
              ElementsAre("example_too.org", "localhost"));
}

}  // namespace adblock
