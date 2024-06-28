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
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {
namespace {

class MockBuffer : public FlatbufferData {
 public:
  MockBuffer()
      : real_data_(
            FlatbufferConverter::Convert({},
                                         GURL{"http://data.com/filters.txt"},
                                         false)) {}
  MOCK_METHOD(void, PermanentlyRemoveSourceOnDestruction, (), (override));

  const uint8_t* data() const override { return real_data_->data(); }
  size_t size() const override { return real_data_->size(); }

  std::unique_ptr<FlatbufferData> real_data_;
};

}  // namespace

class AdblockInstalledSubscriptionImplMetadataTest
    : public AdblockInstalledSubscriptionImplTestBase {};

TEST_F(AdblockInstalledSubscriptionImplMetadataTest, SubscriptionUrlParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules("", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetSourceUrl(), kSubscriptionUrl);
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest, SubscriptionTitleParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules("", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetTitle(), "TestingList");
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest, CurrentVersionParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules(
      "! Version: 202108191113\n !Expires: 1 d", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetCurrentVersion(), "202108191113");
  EXPECT_EQ(subscription->GetExpirationInterval(), base::Days(1));
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest, ConverterRedirects) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  const GURL kRedirectUrl{"https://redirect.url.org/redirect-fl.txt"};
  std::string rules = "[Adblock Plus 2.0]\n! Title: TestingList\n! Redirect: " +
                      kRedirectUrl.spec() + "\n";
  std::stringstream input(std::move(rules));
  auto converter_result =
      FlatbufferConverter::Convert(input, kSubscriptionUrl, false);
  ASSERT_TRUE(absl::holds_alternative<GURL>(converter_result));
  EXPECT_EQ(absl::get<GURL>(converter_result), kRedirectUrl);
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest, InvalidMetadataField) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  std::string rules = "[uBlock Origin 2.0]\n! Title: TestingList\n!";
  std::stringstream input(std::move(rules));
  auto converter_result =
      FlatbufferConverter::Convert(input, kSubscriptionUrl, false);
  ASSERT_TRUE(absl::holds_alternative<ConversionError>(converter_result));
  EXPECT_EQ(absl::get<ConversionError>(converter_result),
            ConversionError("Invalid filter list metadata"));
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest,
       InstallationStateAndDateReported) {
  const auto installation_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Seconds(30));
  const auto installation_state = Subscription::InstallationState::Preloaded;
  auto subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::make_unique<MockBuffer>(), installation_state, installation_time);
  EXPECT_EQ(subscription->GetInstallationState(), installation_state);
  EXPECT_EQ(subscription->GetInstallationTime(), installation_time);
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest,
       MarkForPermanentRemovalTriggersSourceRemoval) {
  auto buffer = std::make_unique<MockBuffer>();
  EXPECT_CALL(*buffer, PermanentlyRemoveSourceOnDestruction());
  auto subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(buffer), Subscription::InstallationState::Installed,
      base::Time());
  subscription->MarkForPermanentRemoval();
}

TEST_F(AdblockInstalledSubscriptionImplMetadataTest,
       NormalDestructionDoesNotTriggerSourceRemoval) {
  auto buffer = std::make_unique<MockBuffer>();
  EXPECT_CALL(*buffer, PermanentlyRemoveSourceOnDestruction()).Times(0);
  auto subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(buffer), Subscription::InstallationState::Installed,
      base::Time());
  subscription.reset();
}

}  // namespace adblock
