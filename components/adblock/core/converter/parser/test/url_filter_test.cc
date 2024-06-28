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

#include "components/adblock/core/converter/parser/url_filter.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;

namespace adblock {

TEST(AdblockUrlFilterTest, ParseEmptyUrlFilter) {
  EXPECT_FALSE(UrlFilter::FromString("").has_value());
}

TEST(AdblockUrlFilterTest, ParseInvalidUrlFilter) {
  EXPECT_FALSE(UrlFilter::FromString("@@").has_value());
  EXPECT_FALSE(UrlFilter::FromString("$valid_option").has_value());
}

TEST(AdblockUrlFilterTest, ParseAllowingUrlFilter) {
  auto url_filter = UrlFilter::FromString("@@allowing_filter");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_TRUE(url_filter->is_allowing);
  EXPECT_EQ(url_filter->pattern, "allowing_filter");
}

TEST(AdblockUrlFilterTest, ParseUrlFilterWithOptions) {
  auto url_filter = UrlFilter::FromString(
      "pattern$csp=script-src: 'none',domain=example.org|~example.com");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_FALSE(url_filter->is_allowing);
  EXPECT_EQ(url_filter->pattern, "pattern");
  ASSERT_TRUE(url_filter->options.Csp().has_value());
  EXPECT_EQ(url_filter->options.Csp(), "script-src: 'none'");
  EXPECT_THAT(url_filter->options.Domains().GetIncludeDomains(),
              ElementsAre("example.org"));
  EXPECT_THAT(url_filter->options.Domains().GetExcludeDomains(),
              ElementsAre("example.com"));
}

TEST(AdblockUrlFilterTest, ParseUrlFilterWithAnInvalidOption) {
  EXPECT_FALSE(UrlFilter::FromString("pattern$invalid_option").has_value());
}

TEST(AdblockUrlFilterTest, ParseBlockingCspFilterWithNoDirectives) {
  EXPECT_FALSE(UrlFilter::FromString("pattern$csp").has_value());
}

TEST(AdblockUrlFilterTest, ParseAllowingCspFilterWithNoDirectives) {
  auto url_filter = UrlFilter::FromString("@@pattern$csp");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_TRUE(url_filter->is_allowing);
  EXPECT_EQ(url_filter->pattern, "pattern");
  ASSERT_TRUE(url_filter->options.Csp().has_value());
  EXPECT_EQ(url_filter->options.Csp(), "");
}

TEST(AdblockUrlFilterTest, ParseBlockingHeaderFilterWithNoDirectives) {
  EXPECT_FALSE(UrlFilter::FromString("pattern$header").has_value());
}

TEST(AdblockUrlFilterTest, ParseRewriteFilterWithDomain) {
  auto url_filter = UrlFilter::FromString(
      "||pattern$rewrite=abp-resource:blank-css,domain=example.hu");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_FALSE(url_filter->is_allowing);
  EXPECT_EQ(url_filter->pattern, "||pattern");
  ASSERT_TRUE(url_filter->options.Rewrite().has_value());
  EXPECT_EQ(url_filter->options.Rewrite(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankCss);
}

TEST(AdblockUrlFilterTest, ParseRewriteFilterWithNoDomainOption) {
  EXPECT_FALSE(UrlFilter::FromString("||pattern$rewrite=abp-resource:blank-css")
                   .has_value());
}

TEST(AdblockUrlFilterTest, ParseRewriteFilterWithNoIncludeDomain) {
  EXPECT_FALSE(
      UrlFilter::FromString(
          "||pattern$rewrite=abp-resource:blank-css,domain=~exclude_domain.com")
          .has_value());
}

TEST(AdblockUrlFilterTest, ParseRewriteFilterWithThirdParty) {
  EXPECT_FALSE(UrlFilter::FromString("||pattern$rewrite=abp-resource:blank-css,"
                                     "domain=example.com,third-party")
                   .has_value());
}

TEST(AdblockUrlFilterTest, ParseRewriteFilterWithInvalidPatter) {
  EXPECT_FALSE(UrlFilter::FromString(
                   "pattern$rewrite=abp-resource:blank-css,domain=example.hu")
                   .has_value());
}

TEST(AdblockUrlFilterTest, ParseBlockingFilterWithExceptionType) {
  EXPECT_FALSE(UrlFilter::FromString("pattern$generichide").has_value());
}

TEST(AdblockUrlFilterTest, ParseAllowingFilterWithoutExceptionType) {
  auto url_filter = UrlFilter::FromString("@@pattern");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_TRUE(url_filter->is_allowing);
  EXPECT_TRUE(url_filter->options.ExceptionTypes().empty());
}

TEST(AdblockUrlFilterTest, ParseAllowingFilterWithExceptionType) {
  auto url_filter = UrlFilter::FromString("@@pattern$generichide");
  ASSERT_TRUE(url_filter.has_value());
  EXPECT_TRUE(url_filter->is_allowing);
  EXPECT_THAT(url_filter->options.ExceptionTypes(),
              ElementsAre(UrlFilterOptions::ExceptionType::Generichide));
}

TEST(AdblockUrlFilterTest, ParseUnspecifcGenericFilter) {
  // Filter too short and too generic, could "break the internet":
  EXPECT_FALSE(UrlFilter::FromString("adv").has_value());
  // |-anchored filters still too short:
  EXPECT_FALSE(UrlFilter::FromString("|adv").has_value());
  EXPECT_FALSE(UrlFilter::FromString("||adv").has_value());
  // Short filter is content-type-specific but not domain-specific:
  EXPECT_FALSE(UrlFilter::FromString("n$image").has_value());

  // Short pattern OK because the filter is domain specific:
  EXPECT_TRUE(UrlFilter::FromString("n$domain=example.com").has_value());
  // Filter pattern long enough:
  EXPECT_TRUE(UrlFilter::FromString("advert").has_value());
  // Filter pattern contains wildcard, allowed to be short:
  EXPECT_TRUE(UrlFilter::FromString("a*").has_value());
}

}  // namespace adblock
