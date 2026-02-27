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

#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplListConverterTest
    : public AdblockInstalledSubscriptionImplTestBase {
 public:
  scoped_refptr<InstalledSubscription> ConvertAndLoadVectorOfRules(
      std::vector<std::string>& rules,
      GURL url = GURL(),
      bool allow_privileged = false) {
    auto raw_data = converter_->Convert(rules, url, allow_privileged);

    auto subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
        std::move(raw_data), Subscription::InstallationState::Installed,
        base::Time());

    EXPECT_EQ(subscription->GetSourceUrl(), "");
    EXPECT_EQ(subscription->GetTitle(), "");
    EXPECT_EQ(subscription->GetCurrentVersion(), "");
    EXPECT_EQ(subscription->GetExpirationInterval(), base::Days(5));

    return subscription;
  }
};

TEST_F(AdblockInstalledSubscriptionImplListConverterTest,
       ConvertEmptyListOfRules) {
  std::vector<std::string> rules = {};
  auto subscription = ConvertAndLoadVectorOfRules(rules);
}

TEST_F(AdblockInstalledSubscriptionImplListConverterTest,
       ConvertListOfRules_SingleRule) {
  std::vector<std::string> rules = {"###ad"};
  auto subscription = ConvertAndLoadVectorOfRules(rules);

  auto selectors = subscription->GetElemhideData(
      GURL("https://pl.ign.com/marvels-avengers/41262/news/"
           "marvels-avengers-kratos-zagra-czarna-pantere"),
      false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>({"#ad"}));
}

TEST_F(AdblockInstalledSubscriptionImplListConverterTest,
       ConvertListOfRulesMultipleRules) {
  std::vector<std::string> rules = {"###ad1", "example.hu###ad2"};
  auto subscription = ConvertAndLoadVectorOfRules(rules);

  auto selectors_1 = subscription->GetElemhideData(
      GURL("https://pl.ign.com/marvels-avengers/41262/news/"
           "marvels-avengers-kratos-zagra-czarna-pantere"),
      false);

  auto selectors_2 =
      subscription->GetElemhideData(GURL("http://example.hu"), false);

  EXPECT_EQ(FilterSelectors(selectors_1), std::set<std::string_view>({"#ad1"}));
  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<std::string_view>({"#ad1", "#ad2"}));
}

TEST_F(AdblockInstalledSubscriptionImplListConverterTest,
       ConvertListOfRulesWithInvalidRules) {
  std::vector<std::string> rules = {"###ad", "", "@@"};
  auto subscription = ConvertAndLoadVectorOfRules(rules);

  auto selectors = subscription->GetElemhideData(
      GURL("https://pl.ign.com/marvels-avengers/41262/news/"
           "marvels-avengers-kratos-zagra-czarna-pantere"),
      false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>({"#ad"}));
}

}  // namespace adblock
