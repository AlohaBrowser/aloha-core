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

#include "components/adblock/core/converter/parser/url_filter_options.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;

namespace adblock {

TEST(AdblockUrlFilterOptionsTest, DefaultOptions) {
  UrlFilterOptions options;
  EXPECT_FALSE(options.IsMatchCase());
  EXPECT_FALSE(options.IsPopup());
  EXPECT_TRUE(options.IsSubresource());
  EXPECT_EQ(options.ThirdParty(), UrlFilterOptions::ThirdPartyOption::Ignore);
  EXPECT_EQ(options.ContentTypes(), ContentType::Default);
  EXPECT_FALSE(options.Rewrite().has_value());
  EXPECT_TRUE(options.Domains().GetIncludeDomains().empty());
  EXPECT_TRUE(options.Domains().GetExcludeDomains().empty());
  EXPECT_TRUE(options.Sitekeys().empty());
  EXPECT_FALSE(options.Csp().has_value());
  EXPECT_FALSE(options.Headers().has_value());
  EXPECT_TRUE(options.ExceptionTypes().empty());
}

TEST(AdblockUrlFilterOptionsTest, ParseEmptyListNoValuesSet) {
  auto options = UrlFilterOptions::FromString("");
  ASSERT_TRUE(options.has_value());
  EXPECT_FALSE(options->IsMatchCase());
  EXPECT_FALSE(options->IsPopup());
  EXPECT_TRUE(options->IsSubresource());
  EXPECT_EQ(options->ThirdParty(), UrlFilterOptions::ThirdPartyOption::Ignore);
  EXPECT_EQ(options->ContentTypes(), ContentType::Default);
  EXPECT_FALSE(options->Rewrite().has_value());
  EXPECT_TRUE(options->Domains().GetIncludeDomains().empty());
  EXPECT_TRUE(options->Domains().GetExcludeDomains().empty());
  EXPECT_TRUE(options->Sitekeys().empty());
  EXPECT_FALSE(options->Csp().has_value());
  EXPECT_FALSE(options->Headers().has_value());
  EXPECT_TRUE(options->ExceptionTypes().empty());
}

TEST(AdblockUrlFilterOptionsTest, ParseMatchCase) {
  EXPECT_TRUE(UrlFilterOptions::FromString("match-case")->IsMatchCase());
  EXPECT_FALSE(UrlFilterOptions::FromString("~match-case")->IsMatchCase());
}

TEST(AdblockUrlFilterOptionsTest, ParsePopupOption) {
  auto options = UrlFilterOptions::FromString("popup");
  EXPECT_TRUE(options->IsPopup());
  EXPECT_FALSE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseThirdParty) {
  auto options = UrlFilterOptions::FromString("third-party");
  EXPECT_EQ(options->ThirdParty(),
            UrlFilterOptions::ThirdPartyOption::ThirdPartyOnly);
  options = UrlFilterOptions::FromString("~third-party");
  EXPECT_EQ(options->ThirdParty(),
            UrlFilterOptions::ThirdPartyOption::FirstPartyOnly);
}

TEST(AdblockUrlFilterOptionsTest, ParseRewriteOptions) {
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-text")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankText);
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-css")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankCss);
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-js")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankJs);
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-html")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankHtml);
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-mp3")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankMp3);
  EXPECT_EQ(UrlFilterOptions::FromString("rewrite=abp-resource:blank-mp4")
                ->Rewrite()
                .value(),
            UrlFilterOptions::RewriteOption::AbpResource_BlankMp4);
  EXPECT_EQ(
      UrlFilterOptions::FromString("rewrite=abp-resource:1x1-transparent-gif")
          ->Rewrite()
          .value(),
      UrlFilterOptions::RewriteOption::AbpResource_TransparentGif1x1);
  EXPECT_EQ(
      UrlFilterOptions::FromString("rewrite=abp-resource:2x2-transparent-png")
          ->Rewrite()
          .value(),
      UrlFilterOptions::RewriteOption::AbpResource_TransparentPng2x2);
  EXPECT_EQ(
      UrlFilterOptions::FromString("rewrite=abp-resource:3x2-transparent-png")
          ->Rewrite()
          .value(),
      UrlFilterOptions::RewriteOption::AbpResource_TransparentPng3x2);
  EXPECT_EQ(
      UrlFilterOptions::FromString("rewrite=abp-resource:32x32-transparent-png")
          ->Rewrite()
          .value(),
      UrlFilterOptions::RewriteOption::AbpResource_TransparentPng32x32);
  EXPECT_FALSE(
      UrlFilterOptions::FromString("rewrite=abp-resource:32x32-transparent-png")
          ->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseInvalidRewriteOptions) {
  EXPECT_FALSE(UrlFilterOptions::FromString("rewrite").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("rewrite=").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("rewrite=invalid-rewrite-option")
                   .has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParseDomainOption) {
  auto options = UrlFilterOptions::FromString(
      "domain=domain.com|~exclude_domain.com|whatever.org");
  ASSERT_TRUE(options.has_value());
  EXPECT_THAT(options->Domains().GetIncludeDomains(),
              ElementsAre("domain.com", "whatever.org"));
  EXPECT_THAT(options->Domains().GetExcludeDomains(),
              ElementsAre("exclude_domain.com"));
}

TEST(AdblockUrlFilterOptionsTest, ParseDomainsInvalid) {
  EXPECT_FALSE(UrlFilterOptions::FromString("domain").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("domain=").has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParsingOptionsIsCaseInsensitive) {
  auto options = UrlFilterOptions::FromString("~ThIRd-PaRtY");
  ASSERT_TRUE(options.has_value());
  EXPECT_EQ(options->ThirdParty(),
            UrlFilterOptions::ThirdPartyOption::FirstPartyOnly);
}

TEST(AdblockUrlFilterOptionsTest, ParseSitekeyOption) {
  auto options = UrlFilterOptions::FromString("sitekey=b|A|D|c");
  ASSERT_TRUE(options.has_value());
  // NOTE: alphabetically ordered, uppercase
  EXPECT_THAT(options->Sitekeys(), ElementsAre(SiteKey{"A"}, SiteKey{"B"},
                                               SiteKey{"C"}, SiteKey{"D"}));
}

TEST(AdblockUrlFilterOptionsTest, ParseInvalidSitekeyOption) {
  EXPECT_FALSE(UrlFilterOptions::FromString("sitekey").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("sitekey=").has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParseCspOption) {
  auto options = UrlFilterOptions::FromString("csp=script-src: 'self'");
  ASSERT_TRUE(options.has_value());
  auto csp = options->Csp();
  ASSERT_TRUE(csp.has_value());
  EXPECT_EQ(csp.value(), "script-src: 'self'");
  EXPECT_FALSE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseEmptyCspOption) {
  auto options = UrlFilterOptions::FromString("csp");
  ASSERT_TRUE(options.has_value());
  auto csp = options->Csp();
  ASSERT_TRUE(csp.has_value());
  EXPECT_EQ(csp.value(), "");
  EXPECT_FALSE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseInvalidCspOption) {
  EXPECT_FALSE(UrlFilterOptions::FromString("csp=report-uri").has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParseHeaderOption) {
  auto options =
      UrlFilterOptions::FromString("header=X-Frame-Options=sameorigin");
  ASSERT_TRUE(options.has_value());
  auto header = options->Headers();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header.value(), "X-Frame-Options=sameorigin");
  EXPECT_FALSE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseEmptyHeaderOption) {
  auto options = UrlFilterOptions::FromString("header");
  ASSERT_TRUE(options.has_value());
  auto header = options->Headers();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header.value(), "");
  EXPECT_FALSE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, InvalidOptionDoesNotGetParsed) {
  EXPECT_FALSE(UrlFilterOptions::FromString("invalid_option").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("invalid=").has_value());
  EXPECT_FALSE(UrlFilterOptions::FromString("invalid=option").has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParseMultipleOptions) {
  auto options = UrlFilterOptions::FromString("csp, third-party");
  ASSERT_TRUE(options.has_value());
  auto csp = options->Csp();
  ASSERT_TRUE(csp.has_value());
  EXPECT_EQ(csp.value(), "");
  EXPECT_EQ(options->ThirdParty(),
            UrlFilterOptions::ThirdPartyOption::ThirdPartyOnly);
}

TEST(AdblockUrlFilterOptionsTest, InvalidOptionMakesOptionsInvalid) {
  EXPECT_FALSE(UrlFilterOptions::FromString("csp, invalid-option").has_value());
}

TEST(AdblockUrlFilterOptionsTest, ParseInvalidOption) {
  EXPECT_FALSE(UrlFilterOptions::FromString("invalid_option"));
}

TEST(AdblockUrlFilterOptionsTest, ParseInverseContentType) {
  auto options = UrlFilterOptions::FromString("~other, other");
  ASSERT_TRUE(options.has_value());
  auto content_types = options->ContentTypes();
  EXPECT_TRUE(content_types & ContentType::Other);
  EXPECT_TRUE(options->IsSubresource());
}

TEST(AdblockUrlFilterOptionsTest, ParseExceptionTypes) {
  auto options =
      UrlFilterOptions::FromString("document, elemhide, generichide");
  ASSERT_TRUE(options.has_value());
  EXPECT_THAT(options->ExceptionTypes(),
              ElementsAre(UrlFilterOptions::ExceptionType::Document,
                          UrlFilterOptions::ExceptionType::Elemhide,
                          UrlFilterOptions::ExceptionType::Generichide));
}

}  // namespace adblock
