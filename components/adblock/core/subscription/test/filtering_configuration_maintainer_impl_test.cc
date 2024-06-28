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

#include "components/adblock/core/subscription/filtering_configuration_maintainer_impl.h"

#include <iterator>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

#include "absl/types/optional.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/gmock_move_support.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/common/header_filter_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/configuration/test/fake_filtering_configuration.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/test/mock_conversion_executors.h"
#include "components/adblock/core/subscription/test/mock_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription_downloader.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/test/mock_subscription_updater.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::NiceMock;

namespace adblock {
namespace {

class FakePersistentStorage final
    : public NiceMock<SubscriptionPersistentStorage> {
 public:
  MOCK_METHOD(void, MockLoadSubscriptions, ());

  void LoadSubscriptions(LoadCallback on_initialized) final {
    on_initialized_ = std::move(on_initialized);
    MockLoadSubscriptions();
  }

  void StoreSubscription(std::unique_ptr<FlatbufferData> raw_data,
                         StoreCallback on_finished) final {
    store_subscription_calls_.emplace_back(std::move(raw_data),
                                           std::move(on_finished));
  }
  void RemoveSubscription(
      scoped_refptr<InstalledSubscription> subscription) final {
    remove_subscription_calls_.push_back(std::move(subscription));
  }

  LoadCallback on_initialized_;
  std::vector<std::pair<std::unique_ptr<FlatbufferData>, StoreCallback>>
      store_subscription_calls_;
  std::vector<scoped_refptr<InstalledSubscription>> remove_subscription_calls_;
};

class FakeBuffer final : public FlatbufferData {
 public:
  const uint8_t* data() const final { return nullptr; }
  size_t size() const final { return 0u; }
};

class FakeSubscription final : public InstalledSubscription {
 public:
  explicit FakeSubscription(
      std::string name,
      InstallationState state = InstallationState::Installed)
      : name_(std::move(name)), state_(state) {}

  GURL GetSourceUrl() const final {
    if (GURL{name_}.is_valid()) {
      return GURL{name_};
    }
    return GURL{"https://easylist-downloads.adblockplus.org/" + name_};
  }
  std::string GetTitle() const final { return name_; }

  std::string GetCurrentVersion() const final { return name_; }

  InstallationState GetInstallationState() const final { return state_; }

  base::Time GetInstallationTime() const final { return base::Time(); }

  base::TimeDelta GetExpirationInterval() const final { return base::Days(5); }

  bool HasUrlFilter(const GURL& url,
                    const std::string& document_domain,
                    ContentType type,
                    const SiteKey& sitekey,
                    FilterCategory category) const final {
    return false;
  }
  bool HasPopupFilter(const GURL& url,
                      const std::string& document_domain,
                      const SiteKey& sitekey,
                      FilterCategory category) const final {
    return false;
  }
  bool HasSpecialFilter(SpecialFilterType type,
                        const GURL& url,
                        const std::string& document_domain,
                        const SiteKey& sitekey) const final {
    return false;
  }
  void FindCspFilters(const GURL& url,
                      const std::string& document_domain,
                      FilterCategory category,
                      std::set<std::string_view>& results) const final {}
  std::set<std::string_view> FindRewriteFilters(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const final {
    return {};
  }
  void FindHeaderFilters(const GURL& url,
                         ContentType type,
                         const std::string& document_domain,
                         FilterCategory category,
                         std::set<HeaderFilterData>& results) const final {}
  ContentFiltersData GetElemhideData(const GURL& url,
                                     bool domain_specific) const final {
    ContentFiltersData result;
    result.elemhide_selectors = {name_};
    return result;
  }
  ContentFiltersData GetElemhideEmulationData(const GURL& url) const final {
    return {};
  }

  std::vector<Snippet> MatchSnippets(
      const std::string& document_domain) const final {
    return {};
  }

  void MarkForPermanentRemoval() final {}

  std::string name_;
  InstallationState state_;

 private:
  ~FakeSubscription() final = default;
};

class MockPreloadedSubscriptionProvider
    : public NiceMock<PreloadedSubscriptionProvider> {
 public:
  MOCK_METHOD(void,
              UpdateSubscriptions,
              (std::vector<GURL> installed_subscriptions,
               std::vector<GURL> pending_subscriptions),
              (override));
  MOCK_METHOD(std::vector<scoped_refptr<InstalledSubscription>>,
              GetCurrentPreloadedSubscriptions,
              (),
              (override, const));
};

}  // namespace

class AdblockFilteringConfigurationMaintainerImplTest : public testing::Test {
 public:
  void CreateTestee(std::vector<scoped_refptr<InstalledSubscription>>
                        demanded_subscriptions) {
    filtering_configuration_ = std::make_unique<FakeFilteringConfiguration>();
    filtering_configuration_->name = "adblock";
    for (auto& sub : demanded_subscriptions) {
      filtering_configuration_->AddFilterList(sub->GetSourceUrl());
    }
    auto storage = std::make_unique<FakePersistentStorage>();
    storage_ = storage.get();
    auto downloader = std::make_unique<MockSubscriptionDownloader>();
    downloader_ = downloader.get();
    auto preloaded_subscription_provider =
        std::make_unique<MockPreloadedSubscriptionProvider>();
    preloaded_subscription_provider_ = preloaded_subscription_provider.get();
    auto updater = std::make_unique<MockSubscriptionUpdater>();
    updater_ = updater.get();

    testee_ = std::make_unique<FilteringConfigurationMaintainerImpl>(
        filtering_configuration_.get(), std::move(storage),
        std::move(downloader), std::move(preloaded_subscription_provider),
        std::move(updater), &conversion_executor_, &persistent_metadata_,
        observer_.Get());
    testee_->InitializeStorage();
  }

  void FinishStorageInitialization(
      std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions) {
    // When storage becomes initialized, testee starts the update checks
    // schedule.
    EXPECT_CALL(*updater_, StartSchedule(_))
        .WillOnce(testing::SaveArg<0>(&run_update_check_callback_));
    std::move(storage_->on_initialized_).Run(loaded_subscriptions);
  }

  void TearDown() override {
    // Avoid danging pointers during destruction.
    storage_ = nullptr;
    preloaded_subscription_provider_ = nullptr;
    updater_ = nullptr;
    downloader_ = nullptr;
  }

  void AddSubscription(
      scoped_refptr<InstalledSubscription> subscription,
      SubscriptionDownloader::RetryPolicy expected_retry_policy =
          SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded) {
    DCHECK(filtering_configuration_) << "Call CreateTestee() first";
    const auto url = subscription->GetSourceUrl();
    // The downloader will be called to fetch the raw_data for subscription.
    EXPECT_CALL(*downloader_,
                StartDownload(url, expected_retry_policy, testing::_))
        .WillOnce([](const GURL&, SubscriptionDownloader::RetryPolicy,
                     base::OnceCallback<void(std::unique_ptr<FlatbufferData>)>
                         callback) {
          // The downloader responds by running the callback with a new
          // buffer, simulating a successful download.
          std::move(callback).Run(std::make_unique<FakeBuffer>());
        });
    filtering_configuration_->AddFilterList(url);

    // Storage was asked to store the buffer provided by downloader.
    ASSERT_EQ(storage_->store_subscription_calls_.size(), 1u);
    EXPECT_TRUE(storage_->store_subscription_calls_[0].first);
    // Storage runs the callback provided by SubscriptionService to indicate
    // store succeeded. This triggers the SubscriptionObserver.
    EXPECT_CALL(observer_, Run(url));
    std::move(storage_->store_subscription_calls_[0].second).Run(subscription);
    storage_->store_subscription_calls_.clear();
  }

  void RemoveSubscription(scoped_refptr<FakeSubscription> subscription) {
    DCHECK(filtering_configuration_) << "Call CreateTestee() first";
    // Simulates a single call to UninstallSubscription that forwards the
    // subscription to storage_ for removal.
    EXPECT_CALL(persistent_metadata_,
                RemoveMetadata(subscription->GetSourceUrl()));
    filtering_configuration_->RemoveFilterList(subscription->GetSourceUrl());
    ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
    EXPECT_EQ(storage_->remove_subscription_calls_[0], subscription);
    storage_->remove_subscription_calls_.clear();
  }

  void InitializeTesteeWithNoSubscriptions() {
    CreateTestee({});
    FinishStorageInitialization({});
  }

  const GURL kRequestUrl{"https://domain.com/resource.jpg"};
  const GURL kParentUrl{"https://domain.com"};
  const SiteKey kSitekey{"abc"};

  std::unique_ptr<FakeFilteringConfiguration> filtering_configuration_;
  raw_ptr<FakePersistentStorage> storage_;
  raw_ptr<MockPreloadedSubscriptionProvider> preloaded_subscription_provider_;
  raw_ptr<MockSubscriptionUpdater> updater_;
  raw_ptr<MockSubscriptionDownloader> downloader_;
  MockSubscriptionPersistentMetadata persistent_metadata_;
  MockConversionExecutors conversion_executor_;
  base::test::TaskEnvironment task_environment_;
  base::MockCallback<
      FilteringConfigurationMaintainerImpl::SubscriptionUpdatedCallback>
      observer_;
  base::RepeatingClosure run_update_check_callback_;
  std::unique_ptr<FilteringConfigurationMaintainerImpl> testee_;
};

TEST_F(AdblockFilteringConfigurationMaintainerImplTest, Initialization) {
  std::vector<scoped_refptr<InstalledSubscription>> initial_subscriptions = {
      base::MakeRefCounted<FakeSubscription>("fake_subscription1"),
      base::MakeRefCounted<FakeSubscription>("fake_subscription2")};
  CreateTestee(initial_subscriptions);

  // Before storage is initialized, no subscriptions are returned.
  EXPECT_TRUE(testee_->GetCurrentSubscriptions().empty());

  // Storage completes initialization, loads two subscriptions.
  FinishStorageInitialization(initial_subscriptions);

  // The subscriptions provided by storage are visible.
  EXPECT_THAT(testee_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(initial_subscriptions[0],
                                            initial_subscriptions[1]));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest, AddSubscription) {
  // Storage has no initial subscriptions:
  InitializeTesteeWithNoSubscriptions();

  // When storage calls its callback, the provided subscription is added to the
  // service and |on_finished| is triggered with the parsed URL.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  AddSubscription(fake_subscription1);

  // Added subscription is reflected in |GetCurrentSubscriptions|.
  EXPECT_THAT(testee_->GetCurrentSubscriptions(),
              testing::ElementsAre(fake_subscription1));

  // The snapshot has a SubscriptionCollection that queries the added
  // subscription. We can check whether FakeSubscription's title appears in
  // Elemhide selectors.
  auto snapshot = testee_->GetSubscriptionCollection();
  auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_THAT(selectors, testing::ElementsAre(fake_subscription1->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       AddMultipleSubscriptionsAndRemoveOne) {
  InitializeTesteeWithNoSubscriptions();

  // Add 3 subscriptions.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  auto fake_subscription3 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription3");
  AddSubscription(fake_subscription1);
  AddSubscription(fake_subscription2);
  AddSubscription(fake_subscription3);

  // Remove middle one.
  RemoveSubscription(fake_subscription2);

  // Two remaining subscription are reflected in |GetInstalledSubscriptions|.
  EXPECT_THAT(
      testee_->GetCurrentSubscriptions(),
      testing::UnorderedElementsAre(fake_subscription1, fake_subscription3));

  // The snapshot has a SubscriptionCollection that queries the remaining
  // subscriptions. We can check whether FakeSubscription's title appears in
  // Elemhide selectors.
  auto snapshot = testee_->GetSubscriptionCollection();
  auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription3->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       SnapshotNotAffectedByFutureAddition) {
  InitializeTesteeWithNoSubscriptions();
  // Add one subscription
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  AddSubscription(fake_subscription1);

  // Take snapshot now.
  auto snapshot = testee_->GetSubscriptionCollection();

  // Add new subscription after snapshot.
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  AddSubscription(fake_subscription2);

  // Snapshot only contains the first subscription.
  auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       SnapshotNotAffectedByFutureRemoval) {
  InitializeTesteeWithNoSubscriptions();
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  AddSubscription(fake_subscription1);
  AddSubscription(fake_subscription2);

  // Take snapshot now.
  auto snapshot = testee_->GetSubscriptionCollection();

  // Remove second subscription.
  RemoveSubscription(fake_subscription2);

  // Snapshot still contains both subscriptions.
  auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription2->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       UpgradeExistingSubscription) {
  InitializeTesteeWithNoSubscriptions();
  auto expired_subscription =
      base::MakeRefCounted<FakeSubscription>("expired_subscription");
  auto young_subscription =
      base::MakeRefCounted<FakeSubscription>("young_subscription");
  AddSubscription(expired_subscription);
  AddSubscription(young_subscription);

  // Pretend one of the subscriptions expired.
  EXPECT_CALL(persistent_metadata_,
              IsExpired(expired_subscription->GetSourceUrl()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(persistent_metadata_,
              IsExpired(young_subscription->GetSourceUrl()))
      .WillRepeatedly(testing::Return(false));
  // Even though Acceptable Ads is not installed, its expiration will be checked
  // to make a HEAD request if needed.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(false));

  // Expect that the expired subscription will be re-downloaded.
  EXPECT_CALL(*downloader_,
              StartDownload(expired_subscription->GetSourceUrl(),
                            SubscriptionDownloader::RetryPolicy::DoNotRetry,
                            testing::_))
      .WillOnce(base::test::RunOnceCallback<2>(std::make_unique<FakeBuffer>()));

  // The young subscription will not be re-downloaded.
  EXPECT_CALL(*downloader_, StartDownload(young_subscription->GetSourceUrl(),
                                          testing::_, testing::_))
      .Times(0);

  run_update_check_callback_.Run();

  // In a second run, even though |expired_subscription| might be marked as
  // expired by persistent_metadata_, there will be no new download since one is
  // already under way.
  EXPECT_CALL(*downloader_, StartDownload(expired_subscription->GetSourceUrl(),
                                          testing::_, testing::_))
      .Times(0);
  run_update_check_callback_.Run();
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       CallOnFilterListsChangedDuringUpgradeExistingSubscriptions) {
  InitializeTesteeWithNoSubscriptions();
  auto expired_subscription =
      base::MakeRefCounted<FakeSubscription>("expired_subscription");
  auto young_subscription =
      base::MakeRefCounted<FakeSubscription>("young_subscription");
  AddSubscription(expired_subscription);
  AddSubscription(young_subscription);

  // Pretend one of the subscriptions expired.
  EXPECT_CALL(persistent_metadata_,
              IsExpired(expired_subscription->GetSourceUrl()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(persistent_metadata_,
              IsExpired(young_subscription->GetSourceUrl()))
      .WillRepeatedly(testing::Return(false));
  // Even though Acceptable Ads is not installed, its expiration will be checked
  // to make a HEAD request if needed.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(false));

  // Expect that the expired subscription will be re-downloaded and call
  // OnFilterListsChanged during update
  EXPECT_CALL(*downloader_,
              StartDownload(expired_subscription->GetSourceUrl(),
                            SubscriptionDownloader::RetryPolicy::DoNotRetry,
                            testing::_))
      .WillOnce(testing::DoAll(
          [&]() {
            testee_->OnFilterListsChanged(filtering_configuration_.get());
          },
          base::test::RunOnceCallback<2>(std::make_unique<FakeBuffer>())));

  // The young subscription will not be re-downloaded.
  EXPECT_CALL(*downloader_, StartDownload(young_subscription->GetSourceUrl(),
                                          testing::_, testing::_))
      .Times(0);

  run_update_check_callback_.Run();

  // In a second run, even though |expired_subscription| might be marked as
  // expired by persistent_metadata_, there will be no new download since one is
  // already under way.
  EXPECT_CALL(*downloader_, StartDownload(expired_subscription->GetSourceUrl(),
                                          testing::_, testing::_))
      .Times(0);
  run_update_check_callback_.Run();

  // After OnFilterListsChanged, check if number of subscriptions is unchanged
  // (DPD-2015)
  EXPECT_EQ(2u, testee_->GetCurrentSubscriptions().size());
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       UpdatePingStoresAAversion) {
  InitializeTesteeWithNoSubscriptions();
  // Once the update check runs, even though Acceptable Ads is not installed,
  // pretend its expired. This will trigger a HEAD request.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(true));

  SubscriptionDownloader::HeadRequestCallback download_completed_callback;
  EXPECT_CALL(*downloader_, DoHeadRequest(AcceptableAdsUrl(), testing::_))
      .WillOnce(MoveArg<1>(&download_completed_callback));

  run_update_check_callback_.Run();

  // When the HEAD request finishes, the service will store the parsed version
  // and the expiration interval.
  const std::string version("202107210821");
  EXPECT_CALL(persistent_metadata_, SetVersion(AcceptableAdsUrl(), version));
  // The next ping should happen in a day.
  EXPECT_CALL(persistent_metadata_,
              SetExpirationInterval(AcceptableAdsUrl(), base::Days(1)));
  std::move(download_completed_callback).Run(version);
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       SendPingOnlyForAdblockConfig) {
  InitializeTesteeWithNoSubscriptions();
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(true));
  SubscriptionDownloader::HeadRequestCallback download_completed_callback;

  // For default "adblock" config ping is sent
  EXPECT_CALL(*downloader_, DoHeadRequest(AcceptableAdsUrl(), testing::_))
      .WillOnce(MoveArg<1>(&download_completed_callback));
  run_update_check_callback_.Run();

  // For non "adblock" config ping is not sent
  filtering_configuration_->name = "crumbs";
  EXPECT_CALL(*downloader_, DoHeadRequest(AcceptableAdsUrl(), testing::_))
      .Times(0);
  run_update_check_callback_.Run();
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       RemoveDuplicatesDuringInitialLoad) {
  // Storage returns 3 subscriptions in initial load, however there is a
  // duplicate, due to a race condition or corruption.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("unique_subscription");
  auto fake_subscription3 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  ASSERT_EQ(fake_subscription1->GetSourceUrl(),
            fake_subscription3->GetSourceUrl());

  CreateTestee({fake_subscription1, fake_subscription2});
  FinishStorageInitialization(
      {fake_subscription1, fake_subscription2, fake_subscription3});

  // Service noticed one subscription is duplicated and it removes one instance
  // - it is unspecified which.
  ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
  EXPECT_EQ(storage_->remove_subscription_calls_[0]->GetSourceUrl(),
            fake_subscription1->GetSourceUrl());

  // Installed subscriptions do not contain duplicates.
  std::vector<GURL> current_subscriptions_urls;
  base::ranges::transform(testee_->GetCurrentSubscriptions(),
                          std::back_inserter(current_subscriptions_urls),
                          [](const auto& sub) { return sub->GetSourceUrl(); });
  EXPECT_THAT(
      current_subscriptions_urls,
      testing::UnorderedElementsAre(fake_subscription1->GetSourceUrl(),
                                    fake_subscription2->GetSourceUrl()));

  // ContentFiltersData returned by snapshot do not contain duplicates.
  const auto snapshot = testee_->GetSubscriptionCollection();

  const auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_EQ(selectors.size(), 2u);
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription2->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       CancellingInstallationDuringDownload_WithPreloadedFallback) {
  // Storage has no initial subscriptions:
  InitializeTesteeWithNoSubscriptions();

  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  const GURL& url = fake_subscription1->GetSourceUrl();

  // SubscriptionDownloader will be called to fetch the subscription. We will
  // trigger the response later, after cancelling installation.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(url, testing::_, testing::_))
      .WillOnce(MoveArg<2>(&download_completed_callback));

  // There is a preloaded fallback available for this URL.
  auto preloaded_subscription = base::MakeRefCounted<FakeSubscription>(
      "fake_subscription1", Subscription::InstallationState::Preloaded);
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{
              preloaded_subscription}));

  // Start installation.
  filtering_configuration_->AddFilterList(url);

  // We should see the preloaded fallback in GetCurrentSubscriptions().
  EXPECT_THAT(testee_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(preloaded_subscription));

  // We now uninstall the subscription, this should cancel the download.
  // The observer is never notified about success.
  EXPECT_CALL(observer_, Run(testing::_)).Times(0);
  // The downloader is told to cancel the download.
  EXPECT_CALL(*downloader_, CancelDownload(url));
  filtering_configuration_->RemoveFilterList(url);

  // The subscription is no longer listed.
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{}));
  EXPECT_TRUE(testee_->GetCurrentSubscriptions().empty());

  // Even when the download callback delivers the FakeBuffer, it will not
  // be sent to storage.
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  // There are no attempts to store the buffer received from Downloader.
  EXPECT_TRUE(storage_->store_subscription_calls_.empty());
  EXPECT_TRUE(testee_->GetCurrentSubscriptions().empty());
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       CancellingInstallationDuringStorage_NoFallback) {
  // Storage has no initial subscriptions:
  InitializeTesteeWithNoSubscriptions();

  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  const GURL& url = fake_subscription1->GetSourceUrl();

  // There are no preloaded fallback available for this URL.
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{}));
  // SubscriptionDownloader will be called to fetch the subscription. It is
  // immediately successful.
  EXPECT_CALL(*downloader_, StartDownload(url, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            std::move(callback).Run(std::make_unique<FakeBuffer>());
          });

  // Start installation.
  filtering_configuration_->AddFilterList(url);

  // The downloader immediately returned a FakeBuffer, it should have been sent
  // to storage.
  ASSERT_EQ(storage_->store_subscription_calls_.size(), 1u);

  // We should see the ongoing installation in GetCurrentSubscriptions().
  const auto current_subscriptions = testee_->GetCurrentSubscriptions();
  ASSERT_EQ(current_subscriptions.size(), 1u);
  EXPECT_EQ(current_subscriptions[0]->GetSourceUrl(), url);
  EXPECT_EQ(current_subscriptions[0]->GetInstallationState(),
            Subscription::InstallationState::Installing);

  // We now uninstall the subscription, this should cancel the installation.
  // The observer is never notified about success.
  EXPECT_CALL(observer_, Run(testing::_)).Times(0);
  filtering_configuration_->RemoveFilterList(url);

  // The subscription is no longer listed.
  EXPECT_TRUE(testee_->GetCurrentSubscriptions().empty());

  // Even when the storage callback delivers the Subscription, it will not
  // be installed in SubscriptionService.
  std::move(storage_->store_subscription_calls_[0].second)
      .Run(fake_subscription1);
  // In fact, the subscription will be scheduled for removal from storage, it
  // is not desired.
  ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
  EXPECT_EQ(storage_->remove_subscription_calls_[0], fake_subscription1);

  EXPECT_TRUE(testee_->GetCurrentSubscriptions().empty());
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest, CustomFilterIsAdded) {
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  InitializeTesteeWithNoSubscriptions();
  AddSubscription(fake_subscription1);

  std::string filter = "test";
  std::vector<std::string> filters = {filter};
  EXPECT_CALL(conversion_executor_, ConvertCustomFilters(filters))
      .WillOnce(testing::Return(
          base::MakeRefCounted<FakeSubscription>(CustomFiltersUrl().spec())));
  filtering_configuration_->AddCustomFilter(filter);

  // The in-memory subscription containing the custom filter is not reported
  // among current subscriptions, only the subscription added by client is.
  EXPECT_THAT(testee_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(fake_subscription1));

  // However, the SubscriptionCollection *does* get the custom filter
  // subscription.
  auto snapshot = testee_->GetSubscriptionCollection();

  auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_THAT(selectors, testing::UnorderedElementsAre(
                             CustomFiltersUrl().spec(), "subscription"));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       PreloadedSubscriptionProviderUpdatedDuringChanges) {
  testing::InSequence sequence;
  InitializeTesteeWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(MoveArg<2>(&download_completed_callback));
  filtering_configuration_->AddFilterList(first_subscription->GetSourceUrl());
  // When download completes, update the provider about new installed
  // subscription, and no pending subscriptions.
  EXPECT_CALL(
      *preloaded_subscription_provider_,
      UpdateSubscriptions(std::vector<GURL>{first_subscription->GetSourceUrl()},
                          std::vector<GURL>{}));

  // Download completes.
  EXPECT_CALL(observer_, Run(first_subscription->GetSourceUrl()));
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  std::move(storage_->store_subscription_calls_.back().second)
      .Run(first_subscription);

  // Second subscription added.
  auto second_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription2");
  // Provider updated with both the old installed subscription and the new
  // ongoing download.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{first_subscription->GetSourceUrl()},
                  std::vector<GURL>{second_subscription->GetSourceUrl()}));

  // Second download starts.
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(MoveArg<2>(&download_completed_callback));

  filtering_configuration_->AddFilterList(second_subscription->GetSourceUrl());

  // When second download completes, provider has two installed and zero pending
  // subscriptions.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{first_subscription->GetSourceUrl(),
                                    second_subscription->GetSourceUrl()},
                  std::vector<GURL>{}));
  EXPECT_CALL(observer_, Run(second_subscription->GetSourceUrl()));
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  std::move(storage_->store_subscription_calls_.back().second)
      .Run(second_subscription);

  // First subscription is uninstalled, provider informed about new state
  // containing only the second subscription.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{second_subscription->GetSourceUrl()},
                  std::vector<GURL>{}));
  filtering_configuration_->RemoveFilterList(
      first_subscription->GetSourceUrl());
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       PreloadedSubscriptionProviderUpdatedDuringFailedDownload) {
  testing::InSequence sequence;
  InitializeTesteeWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(MoveArg<2>(&download_completed_callback));
  filtering_configuration_->AddFilterList(first_subscription->GetSourceUrl());
  // When download fails, inform the provider about returning to previous state.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}));

  // Download fails.
  std::move(download_completed_callback).Run(nullptr);
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       PreloadedSubscriptionProviderUpdatedWhenInstallationCancelled) {
  testing::InSequence sequence;
  InitializeTesteeWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(MoveArg<2>(&download_completed_callback));
  filtering_configuration_->AddFilterList(first_subscription->GetSourceUrl());
  // When installation is cancelled, inform the provider about returning to
  // previous state.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}))
      .Times(testing::AtLeast(1));
  filtering_configuration_->RemoveFilterList(
      first_subscription->GetSourceUrl());

  // Download completes, but the installation was cancelled in the mean time.
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       PreloadedSubscriptionProviderConsultedForSnapshot) {
  auto subscription_in_service =
      base::MakeRefCounted<FakeSubscription>("subscription_in_service");
  auto preloaded_subscription = base::MakeRefCounted<FakeSubscription>(
      "preloaded_subscription", Subscription::InstallationState::Preloaded);
  InitializeTesteeWithNoSubscriptions();
  AddSubscription(subscription_in_service);

  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillOnce(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{
              preloaded_subscription}));

  // Snapshot provides both the subscription in service and the preloaded
  // subscription returned by provider.
  const auto snapshot = testee_->GetSubscriptionCollection();
  const auto selectors =
      snapshot->GetElementHideData(GURL(), {}, SiteKey()).elemhide_selectors;
  EXPECT_EQ(selectors.size(), 2u);
  EXPECT_THAT(selectors, testing::UnorderedElementsAre(
                             subscription_in_service->GetTitle(),
                             preloaded_subscription->GetTitle()));
}

TEST_F(AdblockFilteringConfigurationMaintainerImplTest,
       AcceptableAdsMetadataRetained) {
  auto aa_subscription =
      base::MakeRefCounted<FakeSubscription>("exceptionrules.txt");
  auto easylist_subscription =
      base::MakeRefCounted<FakeSubscription>("easylist.txt");
  InitializeTesteeWithNoSubscriptions();
  AddSubscription(aa_subscription);
  AddSubscription(easylist_subscription);

  // Removing EasyList clears the subscription's metadata.
  EXPECT_CALL(persistent_metadata_,
              RemoveMetadata(easylist_subscription->GetSourceUrl()));
  filtering_configuration_->RemoveFilterList(
      easylist_subscription->GetSourceUrl());

  // Removing the Acceptable Ads subscription retains metadata, in order to
  // allow sending continued HEAD-only update-like requests with consistent
  // expiry date.
  EXPECT_CALL(persistent_metadata_,
              RemoveMetadata(aa_subscription->GetSourceUrl()))
      .Times(0);
  filtering_configuration_->RemoveFilterList(aa_subscription->GetSourceUrl());
}

}  // namespace adblock
