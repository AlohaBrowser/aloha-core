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

#include "components/adblock/core/subscription/preloaded_subscription_provider_impl.h"

#include "base/ranges/algorithm.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class PreloadedSubscriptionProviderImplTest : public testing::Test {
 public:
  base::test::TaskEnvironment task_environment_;
  PreloadedSubscriptionProviderImpl provider_;
};

TEST_F(PreloadedSubscriptionProviderImplTest,
       NoSubscriptionsYieldsNoPreloadedSubscriptions) {
  EXPECT_TRUE(provider_.GetCurrentPreloadedSubscriptions().empty());
  provider_.UpdateSubscriptions({}, {});
  EXPECT_TRUE(provider_.GetCurrentPreloadedSubscriptions().empty());
}

TEST_F(PreloadedSubscriptionProviderImplTest,
       CustomSubscriptionYieldsNoPreloadedSubscriptions) {
  const GURL kCustomSubscription{"https://subs.com/1.txt"};
  provider_.UpdateSubscriptions({}, {kCustomSubscription});
  EXPECT_TRUE(provider_.GetCurrentPreloadedSubscriptions().empty());
}

TEST_F(PreloadedSubscriptionProviderImplTest, EasyListRequired) {
  const GURL kInstalledSubscription{"https://subs.com/1.txt"};
  const GURL kPendingSubscription{
      "https://easylist-downloads.adblockplus.org/easylist.txt"};
  provider_.UpdateSubscriptions({kInstalledSubscription},
                                {kPendingSubscription});
  const auto preloaded_subscriptions =
      provider_.GetCurrentPreloadedSubscriptions();
  ASSERT_EQ(preloaded_subscriptions.size(), 1u);
  EXPECT_EQ(preloaded_subscriptions[0]->GetSourceUrl(),
            DefaultSubscriptionUrl());
  EXPECT_EQ(preloaded_subscriptions[0]->GetInstallationState(),
            Subscription::InstallationState::Preloaded);
}

TEST_F(PreloadedSubscriptionProviderImplTest, ExceptionrulesRequired) {
  const GURL kInstalledSubscription{"https://subs.com/1.txt"};
  const GURL kPendingSubscription{
      "https://easylist-downloads.adblockplus.org/exceptionrules.txt"};
  provider_.UpdateSubscriptions({kInstalledSubscription},
                                {kPendingSubscription});
  const auto preloaded_subscriptions =
      provider_.GetCurrentPreloadedSubscriptions();
  ASSERT_EQ(preloaded_subscriptions.size(), 1u);
  EXPECT_EQ(preloaded_subscriptions[0]->GetSourceUrl(), AcceptableAdsUrl());
}

TEST_F(PreloadedSubscriptionProviderImplTest, AnticvRequired) {
  const GURL kInstalledSubscription{"https://subs.com/1.txt"};
  const GURL kPendingSubscription{
      "https://easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt"};
  provider_.UpdateSubscriptions({kInstalledSubscription},
                                {kPendingSubscription});
  const auto preloaded_subscriptions =
      provider_.GetCurrentPreloadedSubscriptions();
  ASSERT_EQ(preloaded_subscriptions.size(), 1u);
  EXPECT_EQ(preloaded_subscriptions[0]->GetSourceUrl(), AntiCVUrl());
}

TEST_F(PreloadedSubscriptionProviderImplTest,
       LanguageSpecificEasyListRequired) {
  const GURL kInstalledSubscription{"https://subs.com/1.txt"};
  // Even though the required subscription is not exactly easylist.txt, it is
  // based on easylist.txt and so we provide the preloaded subscription.
  const GURL kPendingSubscription{
      "https://easylist-downloads.adblockplus.org/easylistpolish+easylist.txt"};
  provider_.UpdateSubscriptions({kInstalledSubscription},
                                {kPendingSubscription});
  const auto preloaded_subscriptions =
      provider_.GetCurrentPreloadedSubscriptions();
  ASSERT_EQ(preloaded_subscriptions.size(), 1u);
  EXPECT_EQ(preloaded_subscriptions[0]->GetSourceUrl(),
            DefaultSubscriptionUrl());
}

TEST_F(PreloadedSubscriptionProviderImplTest,
       SubscriptionInstalledAndUpdating) {
  const GURL kInstalledSubscription{
      "https://easylist-downloads.adblockplus.org/exceptionrules.txt"};
  const GURL kPendingSubscription{
      "https://easylist-downloads.adblockplus.org/exceptionrules.txt"};
  provider_.UpdateSubscriptions({kInstalledSubscription},
                                {kPendingSubscription});
  const auto preloaded_subscriptions =
      provider_.GetCurrentPreloadedSubscriptions();
  // No need to provide a preloaded subscription because the required
  // subscription is already installed, an update is under way.
  EXPECT_TRUE(provider_.GetCurrentPreloadedSubscriptions().empty());
}

TEST_F(PreloadedSubscriptionProviderImplTest, MultipleRequiredSubscriptions) {
  const GURL kAcceptableAdsSubscription{
      "https://easylist-downloads.adblockplus.org/exceptionrules.txt"};
  const GURL kEasyListPolishSubscription{
      "https://easylist-downloads.adblockplus.org/easylistpolish+easylist.txt"};
  const GURL kAntiCVSubscription{
      "https://easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt"};
  const GURL kOtherInstalledSubscription1{"https://subs.com/1.txt"};
  const GURL kOtherInstalledSubscription2{"https://subs.com/2.txt"};
  const GURL kOtherNonInstalledSubscription1{"https://subs.com/3.txt"};
  provider_.UpdateSubscriptions(
      {kOtherInstalledSubscription1, kOtherInstalledSubscription2},
      {kOtherInstalledSubscription2, kOtherNonInstalledSubscription1,
       kAcceptableAdsSubscription, kEasyListPolishSubscription,
       kAntiCVSubscription});
  auto preloaded_subscriptions = provider_.GetCurrentPreloadedSubscriptions();
  ASSERT_EQ(preloaded_subscriptions.size(), 3u);
  base::ranges::sort(preloaded_subscriptions, {}, &Subscription::GetSourceUrl);
  EXPECT_EQ(preloaded_subscriptions[0]->GetSourceUrl(), AntiCVUrl());
  EXPECT_EQ(preloaded_subscriptions[1]->GetSourceUrl(),
            DefaultSubscriptionUrl());
  EXPECT_EQ(preloaded_subscriptions[2]->GetSourceUrl(), AcceptableAdsUrl());
  EXPECT_EQ(preloaded_subscriptions[0]->GetInstallationState(),
            Subscription::InstallationState::Preloaded);
  EXPECT_EQ(preloaded_subscriptions[1]->GetInstallationState(),
            Subscription::InstallationState::Preloaded);
  EXPECT_EQ(preloaded_subscriptions[2]->GetInstallationState(),
            Subscription::InstallationState::Preloaded);
}

}  // namespace adblock
