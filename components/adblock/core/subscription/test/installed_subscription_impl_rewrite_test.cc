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

#include "components/adblock/core/subscription/test/installed_subscription_impl_test_base.h"

#include "absl/types/variant.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplRewriteTest
    : public AdblockInstalledSubscriptionImplTestBase {};

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteEmpty) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=adform.net,rewrite=
     ||adform.net/banners/scripts/adx.js$domain=adform.net,rewrite
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(
                      GURL("https://adform.net/banners/scripts/adx.js"),
                      "adform.net", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.css$domain=delfi.lt,rewrite=abp-resource:blank-css
    )");
  EXPECT_EQ(*subscriptions
                 ->FindRewriteFilters(
                     GURL("https://adform.net/banners/scripts/adx.css"),
                     "delfi.lt", FilterCategory::Blocking)
                 .begin(),
            "data:text/css,");
  EXPECT_EQ(*subscriptions
                 ->FindRewriteFilters(
                     GURL("https://adform.net/banners/scripts/adx.css"),
                     "delfi.lt", FilterCategory::DomainSpecificBlocking)
                 .begin(),
            "data:text/css,");
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteFirstParty) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.css$rewrite=abp-resource:blank-css,~third-party
    )");
  EXPECT_EQ(*subscriptions
                 ->FindRewriteFilters(
                     GURL("https://adform.net/banners/scripts/adx.css"),
                     "adform.net", FilterCategory::Blocking)
                 .begin(),
            "data:text/css,");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(
                      GURL("https://adform.net/banners/scripts/adx.css"),
                      "adform.net", FilterCategory::DomainSpecificBlocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest,
       RewriteFirstPartyDomainNotMatching) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.css$rewrite=abp-resource:blank-css,~third-party
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(
                      GURL("https://adform.net/banners/scripts/adx.css"),
                      "example.com", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest,
       RewriteFirstPartyAndDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=adform.net,rewrite=abp-resource:blank-js,~third-party
    )");
  EXPECT_EQ(*subscriptions
                 ->FindRewriteFilters(
                     GURL("https://adform.net/banners/scripts/adx.js"),
                     "adform.net", FilterCategory::Blocking)
                 .begin(),
            "data:application/javascript,");
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteWrongResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=delfi.lt,rewrite=abp-resource:blank-xxx
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(
                      GURL("https://adform.net/banners/scripts/adx.js"),
                      "delfi.lt", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteWrongScheme) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=delfi.lt,rewrite=about::blank
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(
                      GURL("https://adform.net/banners/scripts/adx.js"),
                      "delfi.lt", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteStar) {
  auto subscriptions = ConvertAndLoadRules(R"(
     *$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_EQ(
      *subscriptions
           ->FindRewriteFilters(
               GURL("https://adform.net/banners/scripts/adx.html"), "delfi.lt",
               FilterCategory::Blocking)
           .begin(),
      "data:text/html,<!DOCTYPE html><html><head></head><body></body></html>");
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteWrongStar) {
  auto subscriptions = ConvertAndLoadRules(R"(
     *test.com$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(GURL("https://test.com/ad.html"),
                                       "delfi.lt", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteStrict) {
  auto subscriptions = ConvertAndLoadRules(R"(
     test.com$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(GURL("https://test.com/ad.html"),
                                       "delfi.lt", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteNoDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||test.com$rewrite=abp-resource:blank-html
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(GURL("https://test.com/ad.html"),
                                       "test.com", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteExcludeDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||test.com$domain=~delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_TRUE(subscriptions
                  ->FindRewriteFilters(GURL("https://test.com/ad.html"),
                                       "test.com", FilterCategory::Blocking)
                  .empty());
}

TEST_F(AdblockInstalledSubscriptionImplRewriteTest, RewriteAllowedFilter) {
  auto subscriptions = ConvertAndLoadRules(R"(
     @@||adform.net/banners/scripts/adx.css$domain=delfi.lt,rewrite=abp-resource:blank-css
    )");
  EXPECT_EQ(*subscriptions
                 ->FindRewriteFilters(
                     GURL("https://adform.net/banners/scripts/adx.css"),
                     "delfi.lt", FilterCategory::Allowing)
                 .begin(),
            "data:text/css,");
}

}  // namespace adblock
