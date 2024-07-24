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

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplCSPTest
    : public AdblockInstalledSubscriptionImplTestBase {
 public:
};

TEST_F(AdblockInstalledSubscriptionImplCSPTest, CspFilterForUrl) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    )");

  std::set<std::string_view> filters1;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking, filters1);
  EXPECT_THAT(filters1, testing::UnorderedElementsAre("script-src 'self'"));

  // Different URL, not found.
  std::set<std::string_view> filters2;
  subscriptions->FindCspFilters(GURL("https://test.org/resource.jpg"),
                                "test.com", FilterCategory::Blocking, filters2);
  EXPECT_TRUE(filters2.empty());

  // Allowing filter, not found.
  std::set<std::string_view> filters3;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Allowing, filters3);
  EXPECT_TRUE(filters3.empty());
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, MultipleCspFiltersForUrl) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'first'
    test.com^$csp=script-src 'second'
    )");

  std::set<std::string_view> filters1;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking, filters1);
  EXPECT_THAT(filters1, testing::UnorderedElementsAre(
                            std::string_view("script-src 'first'"),
                            std::string_view("script-src 'second'")));

  // Different URL, not found.
  std::set<std::string_view> filters2;
  subscriptions->FindCspFilters(GURL("https://test.org/resource.jpg"),
                                "test.com", FilterCategory::Blocking, filters2);
  EXPECT_TRUE(filters2.empty());

  // Allowing filter, not found.
  std::set<std::string_view> filters3;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Allowing, filters3);
  EXPECT_TRUE(filters3.empty());
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, CspFilterForDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    $csp=script-src 'self' '*' 'unsafe-inline',domain=dom_a.com|dom_b.com
    )");

  std::set<std::string_view> filters1;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "dom_b.com", FilterCategory::Blocking,
                                filters1);
  EXPECT_THAT(filters1, testing::UnorderedElementsAre(
                            "script-src 'self' '*' 'unsafe-inline'"));

  std::set<std::string_view> filters2;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "dom_a.com", FilterCategory::Blocking,
                                filters2);
  EXPECT_THAT(filters2, testing::UnorderedElementsAre(
                            "script-src 'self' '*' 'unsafe-inline'"));

  std::set<std::string_view> filters3;
  subscriptions->FindCspFilters(GURL("https://dom_a.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking, filters3);
  EXPECT_TRUE(filters3.empty());
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, AllowingCspFilterNoPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    @@test.com^$csp
    )");

  std::set<std::string_view> blocking_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking,
                                blocking_filters);
  EXPECT_THAT(blocking_filters,
              testing::UnorderedElementsAre("script-src 'self'"));

  // Allowing filter is found, with an empty payload.
  std::set<std::string_view> allowing_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Allowing,
                                allowing_filters);
  EXPECT_THAT(allowing_filters, testing::UnorderedElementsAre(""));
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, AllowingCspFilterWithPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    @@test.com^$csp=script-src 'self'
    )");

  std::set<std::string_view> blocking_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking,
                                blocking_filters);
  EXPECT_THAT(blocking_filters,
              testing::UnorderedElementsAre("script-src 'self'"));

  // Allowing filter is found, with payload.
  std::set<std::string_view> allowing_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Allowing,
                                allowing_filters);
  EXPECT_THAT(allowing_filters,
              testing::UnorderedElementsAre("script-src 'self'"));
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest,
       MultipleAllowingCspFiltersWithPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    @@test.com^$csp=script-src 'first'
    @@test.com^$csp=script-src 'second'
    )");

  std::set<std::string_view> expected_blocking{"script-src 'self'"};

  std::set<std::string_view> blocking_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking,
                                blocking_filters);
  EXPECT_EQ(expected_blocking, blocking_filters);

  // Allowing filter is found, with payload.
  std::set<std::string_view> expected_allowing{"script-src 'first'",
                                               "script-src 'second'"};

  std::set<std::string_view> allowing_filters;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Allowing,
                                allowing_filters);
  EXPECT_EQ(expected_allowing, allowing_filters);
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, DomainSpecificOnlyCspFilter) {
  // This filter is not domain-specific.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    )");

  // It's not found when domain_specific_only = true.
  std::set<std::string_view> results;
  subscriptions->FindCspFilters(
      GURL("https://test.com/resource.jpg"), "test.com",
      FilterCategory::DomainSpecificBlocking, results);
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest, ThirdPartyCspFilters) {
  auto subscriptions = ConvertAndLoadRules(R"(
    only-third.com^$csp=only-third,third-party
    never-third.com^$csp=never-third,~third-party
    )");

  // only-third is only found when the URL is from a different domain.

  std::set<std::string_view> filters1;
  subscriptions->FindCspFilters(GURL("https://only-third.com/resource.jpg"),
                                "only-third.com", FilterCategory::Blocking,
                                filters1);
  EXPECT_TRUE(filters1.empty());

  // never-third is only found when the URL is from the same domain.
  std::set<std::string_view> filters2;
  subscriptions->FindCspFilters(GURL("https://never-third.com/resource.jpg"),
                                "never-third.com", FilterCategory::Blocking,
                                filters2);
  EXPECT_THAT(filters2, testing::UnorderedElementsAre("never-third"));

  std::set<std::string_view> results3;
  subscriptions->FindCspFilters(GURL("https://never-third.com/resource.jpg"),
                                "different.com", FilterCategory::Blocking,
                                results3);
  EXPECT_TRUE(results3.empty());
}

TEST_F(AdblockInstalledSubscriptionImplCSPTest,
       BlockingCspFilterWithoutPayloadIgnored) {
  // It's impossible to say what CSP header should be injected if the filter
  // doesn't specify.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp
    )");

  std::set<std::string_view> results;
  subscriptions->FindCspFilters(GURL("https://test.com/resource.jpg"),
                                "test.com", FilterCategory::Blocking, results);
  EXPECT_TRUE(results.empty());
}

// TODO(mpawlowski) support multiple CSP filters per URL + frame hierarchy:
// DPD-1145.

}  // namespace adblock
