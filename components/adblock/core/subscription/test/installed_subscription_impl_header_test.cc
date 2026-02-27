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

#include "absl/types/variant.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/test/installed_subscription_impl_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplHeaderTest
    : public AdblockInstalledSubscriptionImplTestBase {};

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       HeaderFilterIgnoredForNonPriviledged) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, false);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       BlockingHeaderFilterWithoutPayloadIgnored) {
  // It's impossible to say if request should be blocked if the filter
  // doesn't specify disallowed header.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest, HeaderFilterForUrl) {
  GURL subscription_url{"url.com"};
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, true);
  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=sameorigin", subscription_url}));

  blocking_filters.clear();
  // Different URL, not found.
  subscriptions->FindHeaderFilters(GURL("https://test.org/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       NoDomainSpecificHeaderFilter) {
  // This filter is not domain-specific.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  // It's not found when domain_specific_only = true.
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "test.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest, DomainSpecificHeaderFilter) {
  auto subscriptions = ConvertAndLoadRules(R"(
    $header=X-Frame-Options=sameorigin,domain=dom-a.com|dom-b.com
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "dom-b.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "dom-a.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(
      GURL("https://dom-a.com/resource.jpg"), ContentType::Image, "test.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       HeaderFilterForSpecificResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://dom-a.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       HeaderFilterForMultipleSpecificResources) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=sameorigin
    test.com^$xmlhttprequest,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.xml"),
                                   ContentType::Xmlhttprequest, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://dom-a.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest, HeaderFilterWithComma) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=same\x2corigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=same,origin", GURL()}));
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=same\x2corigin", GURL()}));
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       HeaderFilterWithx2cAsPartOfFilter) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=same\\x2corigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  // This count  "X-Frame-Options=same\x2corigin" occurrences, extra \ is
  // omitted during string construction
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=same\\x2corigin", GURL()}));
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       AllowingHeaderFilterNoPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$header
    )",
                                           {}, true);

  // Allowing filter is found, with an empty payload.
  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"", GURL()}));
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       AllowingHeaderFilterWithPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$header=X-Frame-Options=value1
    )",
                                           {}, true);

  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"X-Frame-Options=value1", GURL()}));
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest,
       AllowingHeaderFilterForSpecificResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$script,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  allowing_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(0u, allowing_filters.size());
}

TEST_F(AdblockInstalledSubscriptionImplHeaderTest, ThirdPartyHeaderFilters) {
  auto subscriptions = ConvertAndLoadRules(R"(
    only-third.com^$header=only-third,third-party
    never-third.com^$header=never-third,~third-party
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  // only-third is only found when the URL is from a different domain.
  subscriptions->FindHeaderFilters(GURL("https://only-third.com/resource.jpg"),
                                   ContentType::Image, "different.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"only-third", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://only-third.com/resource.jpg"),
                                   ContentType::Image, "only-third.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());

  // never-third is only found when the URL is from the same domain.
  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://never-third.com/resource.jpg"),
                                   ContentType::Image, "never-third.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"never-third", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://never-third.com/resource.jpg"),
                                   ContentType::Image, "different.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

}  // namespace adblock
