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

#include "components/adblock/core/converter/parser/content_filter.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using ::testing::UnorderedElementsAre;

TEST(AdblockContentFilterTest, ParseEmptyContentFilter) {
  EXPECT_FALSE(
      ContentFilter::FromString("", FilterType::ElemHide, "").has_value());
}

TEST(AdblockContentFilterTest, ParseElemHideFilter) {
  auto content_filter = ContentFilter::FromString(
      "example.org", FilterType::ElemHide,
      ".testcase-container > .testcase-eh-descendant");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHide);
  EXPECT_EQ(content_filter->selector,
            ".testcase-container > .testcase-eh-descendant");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideFilterWithNonAsciiCharacters) {
  // Non-ASCII characters are allowed in selectors. They should be preserved.
  auto content_filter =
      ContentFilter::FromString("test.com", FilterType::ElemHide, ".ad_bÖx");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHide);
  EXPECT_EQ(content_filter->selector, ".ad_bÖx");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("test.com"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideFilterMultipleDomains) {
  auto content_filter =
      ContentFilter::FromString("example.org,~foo.example.org,bar.example.org",
                                FilterType::ElemHide, "test-selector");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHide);
  EXPECT_EQ(content_filter->selector, "test-selector");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org", "bar.example.org"));
  EXPECT_THAT(content_filter->domains.GetExcludeDomains(),
              UnorderedElementsAre("foo.example.org"));
}

TEST(AdblockContentFilterTest, ParseElemHideFilterWithIdSelector) {
  auto content_filter = ContentFilter::FromString(
      "example.org", FilterType::ElemHide, "#this_is_an_id");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHide);
  EXPECT_EQ(content_filter->selector, "#this_is_an_id");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideFilterWithNoDomains) {
  auto content_filter =
      ContentFilter::FromString("", FilterType::ElemHide, "selector");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHide);
  EXPECT_EQ(content_filter->selector, "selector");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_TRUE(content_filter->domains.GetIncludeDomains().empty());
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideFilterWithNoSelector) {
  ASSERT_FALSE(
      ContentFilter::FromString("example.org", FilterType::ElemHide, "")
          .has_value());
}

TEST(AdblockContentFilterTest, ParseElemHideExceptionFilter) {
  auto content_filter = ContentFilter::FromString(
      "example.org", FilterType::ElemHideException, "selector");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideException);
  EXPECT_EQ(content_filter->selector, "selector");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideExceptionFilterWithIdSelector) {
  auto content_filter = ContentFilter::FromString(
      "example.org", FilterType::ElemHideException, "#this_is_an_id");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideException);
  EXPECT_EQ(content_filter->selector, "#this_is_an_id");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideEmulationFilter) {
  auto content_filter = ContentFilter::FromString(
      "foo.example.org", FilterType::ElemHideEmulation, "foo");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideEmulation);
  EXPECT_EQ(content_filter->selector, "foo");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("foo.example.org"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest,
     ParseElemHideEmulationFilterWithNonAsciiCharacters) {
  // Non-ASCII characters are allowed in selectors. They should be preserved.
  auto content_filter = ContentFilter::FromString(
      "test.com", FilterType::ElemHideEmulation, ".ad_bÖx");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideEmulation);
  EXPECT_EQ(content_filter->selector, ".ad_bÖx");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("test.com"));
  EXPECT_TRUE(content_filter->domains.GetExcludeDomains().empty());
}

TEST(AdblockContentFilterTest, ParseElemHideEmulationFilterNoIncludeDomain) {
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org",
                                         FilterType::ElemHideEmulation, "foo")
                   .has_value());
}

TEST(AdblockContentFilterTest,
     ParseElemHideEmulationFilterDomainsWithNoSubdomainRemoved) {
  auto content_filter =
      ContentFilter::FromString("org,example.org,~com,~example_too.org",
                                FilterType::ElemHideEmulation, "foo");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideEmulation);
  EXPECT_EQ(content_filter->selector, "foo");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_THAT(content_filter->domains.GetExcludeDomains(),
              UnorderedElementsAre("example_too.org"));
}

TEST(AdblockContentFilterTest,
     ParseElemHideEmulationFilterDomainsWithNoSubdomainRemovedButNotLocalhost) {
  auto content_filter =
      ContentFilter::FromString("org,example.org,~localhost,~example_too.org",
                                FilterType::ElemHideEmulation, "foo");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::ElemHideEmulation);
  EXPECT_EQ(content_filter->selector, "foo");
  EXPECT_TRUE(content_filter->modifier.empty());
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_THAT(content_filter->domains.GetExcludeDomains(),
              UnorderedElementsAre("example_too.org", "localhost"));
}

TEST(AdblockContentFilterTest, ParseUnspecifcContentFilter) {
  // Short, non domain-specific filters are disallowed because they could break
  // a lot of sites by accident:
  EXPECT_FALSE(
      ContentFilter::FromString("", FilterType::ElemHide, "li").has_value());
  EXPECT_FALSE(ContentFilter::FromString("", FilterType::ElemHideException, "p")
                   .has_value());

  // This filter is long enough:
  EXPECT_TRUE(
      ContentFilter::FromString("", FilterType::ElemHide, "adv").has_value());
  // This filter is short ("p") but domain-specific, so in worst case it could
  // only break example.com.
  EXPECT_TRUE(
      ContentFilter::FromString("example.com", FilterType::ElemHide, "p")
          .has_value());
}

TEST(AdblockContentFilterTest, ParseRemoveFilter) {
  auto content_filter = ContentFilter::FromString(
      "example.org,~localhost", FilterType::Remove, "foo {remove: true;}");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::Remove);
  EXPECT_EQ(content_filter->selector, "foo");
  EXPECT_EQ(content_filter->modifier, "remove: true;");
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_THAT(content_filter->domains.GetExcludeDomains(),
              UnorderedElementsAre("localhost"));
}

TEST(AdblockContentFilterTest, ParseInlineCssFilter) {
  auto content_filter =
      ContentFilter::FromString("example.org,~localhost", FilterType::InlineCss,
                                "foo { some inline: css; }");
  ASSERT_TRUE(content_filter.has_value());
  EXPECT_EQ(content_filter->type, FilterType::InlineCss);
  EXPECT_EQ(content_filter->selector, "foo");
  EXPECT_EQ(content_filter->modifier, "some inline: css;");
  EXPECT_THAT(content_filter->domains.GetIncludeDomains(),
              UnorderedElementsAre("example.org"));
  EXPECT_THAT(content_filter->domains.GetExcludeDomains(),
              UnorderedElementsAre("localhost"));
}

TEST(AdblockContentFilterTest,
     ParseRemoveAndInlineCssFilterWithNoPseudoSelector) {
  ASSERT_FALSE(
      ContentFilter::FromString("~foo.example.org", FilterType::Remove, "foo")
          .has_value());
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org",
                                         FilterType::InlineCss, "foo")
                   .has_value());
}

TEST(AdblockContentFilterTest,
     ParseRemoveAndInlineCssFilterWithInvalidPseudoSelector) {
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org", FilterType::Remove,
                                         "foo remove: true;}")
                   .has_value());
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org",
                                         FilterType::InlineCss,
                                         "foo   inline_css; }")
                   .has_value());
}

TEST(AdblockContentFilterTest, ParseRemoveAndInlineCssFilterWithNoSelector) {
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org", FilterType::Remove,
                                         "{remove: true;}")
                   .has_value());
  ASSERT_FALSE(ContentFilter::FromString("~foo.example.org",
                                         FilterType::InlineCss, "{inline_css;}")
                   .has_value());
}

}  // namespace adblock
