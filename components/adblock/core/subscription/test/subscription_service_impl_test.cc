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

#include "components/adblock/core/subscription/subscription_service_impl.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/configuration/test/fake_filtering_configuration.h"
#include "components/adblock/core/subscription/filtering_configuration_maintainer.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/test/mock_filtering_configuration_maintainer.h"
#include "components/adblock/core/subscription/test/mock_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/prefs/testing_pref_service.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::NiceMock;
using testing::Return;

namespace adblock {
namespace {
class MockSubscriptionObserver
    : public NiceMock<SubscriptionService::SubscriptionObserver> {
 public:
  MOCK_METHOD(void,
              OnSubscriptionInstalled,
              (const GURL& subscription_url),
              (override));
};
}  // namespace

class AdblockSubscriptionServiceImplTest : public testing::Test {
 public:
  struct MaintainerFactoryCall {
    raw_ptr<FilteringConfiguration> input_configuration;
    SubscriptionServiceImpl::SubscriptionUpdatedCallback input_update_callback;
    raw_ptr<MockFilteringConfigurationMaintainer> output_maintainer;
  };

  AdblockSubscriptionServiceImplTest()
      : testee_(
            &prefs_,
            base::BindRepeating(
                &AdblockSubscriptionServiceImplTest::MockMakeMaintainer,
                base::Unretained(this)),
            base::BindRepeating(
                &AdblockSubscriptionServiceImplTest::MockCleanupConfiguration,
                base::Unretained(this))) {}

  void TearDown() override {
    // Avoid dangling pointers during destruction.
    maintainer_factory_calls_.clear();
  }

  void DisableConfigurationAndEnsureMaintainerDestroyed(
      MaintainerFactoryCall* entry) {
    bool maintainer_destroyed = false;
    EXPECT_CALL(*entry->output_maintainer, Destructor()).WillOnce([&]() {
      maintainer_destroyed = true;
    });
    entry->output_maintainer = nullptr;
    entry->input_configuration->SetEnabled(false);
    // Explicitly verifying the maintainer was destroyed now, because the
    // destructor will always be called *eventually*. We want to make sure it
    // was called in response to SetEnabled(false).
    EXPECT_TRUE(maintainer_destroyed);
  }

  std::unique_ptr<FilteringConfigurationMaintainer> MockMakeMaintainer(
      FilteringConfiguration* configuration,
      SubscriptionServiceImpl::SubscriptionUpdatedCallback update_callback) {
    auto maintainer = std::make_unique<MockFilteringConfigurationMaintainer>();
    maintainer_factory_calls_.push_back(
        {configuration, update_callback, maintainer.get()});
    return maintainer;
  }

  void MockCleanupConfiguration(FilteringConfiguration* configuration) {
    cleaned_configs_.push_back(configuration->GetName());
  }

  bool IsAnEmptySubscription(
      const scoped_refptr<adblock::Subscription>& subscription) {
    return (subscription->GetInstallationState() ==
            InstalledSubscription::InstallationState::Unknown) &&
           subscription->GetTitle().empty() &&
           subscription->GetCurrentVersion().empty();
  }

  std::vector<MaintainerFactoryCall> maintainer_factory_calls_;
  std::vector<std::string> cleaned_configs_;
  MockSubscriptionObserver observer_;
  TestingPrefServiceSimple prefs_;
  SubscriptionServiceImpl testee_;
};

TEST_F(AdblockSubscriptionServiceImplTest, EmptySnapshotWithoutConfigurations) {
  EXPECT_TRUE(testee_.GetCurrentSnapshot().empty());
  EXPECT_TRUE(testee_.GetInstalledFilteringConfigurations().empty());
}

TEST_F(AdblockSubscriptionServiceImplTest,
       InstallingDisabledConfigurationDoesNotCreateMaintainer) {
  auto config = std::make_unique<FakeFilteringConfiguration>();
  config->is_enabled = false;
  auto* config_bare_ptr = config.get();
  testee_.InstallFilteringConfiguration(std::move(config));

  EXPECT_TRUE(maintainer_factory_calls_.empty());
  EXPECT_EQ(testee_.GetInstalledFilteringConfigurations(),
            std::vector<FilteringConfiguration*>{config_bare_ptr});
}

TEST_F(AdblockSubscriptionServiceImplTest,
       InstallingEnabledConfigurationCreatesMaintainer) {
  auto config = std::make_unique<FakeFilteringConfiguration>();
  auto* config_bare_ptr = config.get();
  config->is_enabled = true;
  testee_.InstallFilteringConfiguration(std::move(config));

  ASSERT_EQ(maintainer_factory_calls_.size(), 1u);
  EXPECT_EQ(maintainer_factory_calls_[0].input_configuration, config_bare_ptr);
  EXPECT_EQ(testee_.GetInstalledFilteringConfigurations(),
            std::vector<FilteringConfiguration*>{config_bare_ptr});
}

TEST_F(AdblockSubscriptionServiceImplTest,
       UninstallingConfigurationRemovesData) {
  auto config = std::make_unique<FakeFilteringConfiguration>();
  auto config_name = config->GetName();
  auto* config_bare_ptr = config.get();
  // Install and confirm.
  testee_.InstallFilteringConfiguration(std::move(config));
  EXPECT_EQ(testee_.GetInstalledFilteringConfigurations(),
            std::vector<FilteringConfiguration*>{config_bare_ptr});
  ASSERT_EQ(0u, cleaned_configs_.size());
  // Avoid dangling pointers after UninstallFilteringConfiguration().
  maintainer_factory_calls_[0].input_configuration = nullptr;
  maintainer_factory_calls_[0].output_maintainer = nullptr;
  // Uninstall and confirm.
  testee_.UninstallFilteringConfiguration(config_name);
  EXPECT_TRUE(testee_.GetInstalledFilteringConfigurations().empty());
  ASSERT_EQ(1u, cleaned_configs_.size());
  EXPECT_EQ(cleaned_configs_[0], config_name);
}

TEST_F(AdblockSubscriptionServiceImplTest,
       EnablingConfigurationCreatesMaintainer) {
  auto config = std::make_unique<FakeFilteringConfiguration>();
  auto* config_bare_ptr = config.get();
  config->is_enabled = false;
  testee_.InstallFilteringConfiguration(std::move(config));

  EXPECT_TRUE(maintainer_factory_calls_.empty());
  config_bare_ptr->SetEnabled(true);

  ASSERT_EQ(maintainer_factory_calls_.size(), 1u);
  EXPECT_EQ(maintainer_factory_calls_[0].input_configuration, config_bare_ptr);
}

TEST_F(AdblockSubscriptionServiceImplTest,
       DisablingConfigurationDestroysMaintainer) {
  auto config = std::make_unique<FakeFilteringConfiguration>();
  auto* config_bare_ptr = config.get();
  config->is_enabled = true;
  testee_.InstallFilteringConfiguration(std::move(config));

  DisableConfigurationAndEnsureMaintainerDestroyed(
      &maintainer_factory_calls_[0]);
  // The configuration remains installed, even if it is disabled and there is
  // no maintainer for it.
  EXPECT_EQ(testee_.GetInstalledFilteringConfigurations(),
            std::vector<FilteringConfiguration*>{config_bare_ptr});
}

TEST_F(AdblockSubscriptionServiceImplTest,
       EnabledMaintainersConsultedForSubscriptions) {
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("1"));
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("2"));
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("3"));

  ASSERT_EQ(maintainer_factory_calls_.size(), 3u);
  EXPECT_THAT(testee_.GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(
                  maintainer_factory_calls_[0].input_configuration,
                  maintainer_factory_calls_[1].input_configuration,
                  maintainer_factory_calls_[2].input_configuration));
  auto disabled_entry = std::move(maintainer_factory_calls_[1]);
  // We disable one configuration. The maintainer of that configuration will
  // be destroyed, so it won't be consulted for subscriptions.
  DisableConfigurationAndEnsureMaintainerDestroyed(&disabled_entry);
  EXPECT_TRUE(
      testee_.GetCurrentSubscriptions(disabled_entry.input_configuration)
          .empty());
  // A maintainer of the wrong configuration is not consulted, even if it's
  // enabled.
  auto& entry_of_different_config = maintainer_factory_calls_[2];
  EXPECT_CALL(*entry_of_different_config.output_maintainer,
              GetCurrentSubscriptions())
      .Times(0);
  // The maintainer of the right configuration is consulted for Subscriptions
  auto& entry_of_config_in_question = maintainer_factory_calls_[0];
  // Set subscriptions in prefs.
  auto url1 = GURL{"https://subscription/1.txt"};
  auto url2 = GURL{"https://subscription/2.txt"};
  auto subscription1 = MakeMockSubscription(url1);
  auto subscription2 = MakeMockSubscription(url2);
  entry_of_config_in_question.input_configuration->AddFilterList(url1);
  entry_of_config_in_question.input_configuration->AddFilterList(url2);
  EXPECT_CALL(*entry_of_config_in_question.output_maintainer,
              GetCurrentSubscriptions())
      .WillOnce(Return(std::vector<scoped_refptr<Subscription>>{
          subscription1, subscription2}));

  EXPECT_THAT(testee_.GetCurrentSubscriptions(
                  entry_of_config_in_question.input_configuration),
              testing::UnorderedElementsAre(subscription1, subscription2));
}

TEST_F(AdblockSubscriptionServiceImplTest,
       GetCurrentSubscriptionsWhenConfigurationDisabled) {
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>());

  ASSERT_EQ(maintainer_factory_calls_.size(), 1u);
  EXPECT_THAT(testee_.GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(
                  maintainer_factory_calls_[0].input_configuration));
  auto disabled_entry = std::move(maintainer_factory_calls_[0]);
  // Set subscriptions in prefs.
  auto url1 = GURL{"https://subscription/1.txt"};
  auto url2 = GURL{"https://subscription/2.txt"};
  disabled_entry.input_configuration->AddFilterList(url1);
  disabled_entry.input_configuration->AddFilterList(url2);
  // We disable one configuration. The maintainer of that configuration will
  // be destroyed, so it won't be consulted for subscriptions.
  DisableConfigurationAndEnsureMaintainerDestroyed(&disabled_entry);
  auto test_result =
      testee_.GetCurrentSubscriptions(disabled_entry.input_configuration);
  EXPECT_EQ(2u, test_result.size());
  for (const GURL& url : {url1, url2}) {
    auto it = base::ranges::find_if(test_result, [&](const auto& entry) {
      return entry->GetSourceUrl() == url;
    });
    ASSERT_TRUE(it != test_result.end());
    EXPECT_TRUE(IsAnEmptySubscription(*it));
  }
}

// Test when maintainer is not yet initialized, meaning storage is
// not yet initialized. This test takes some shortcut and does not mock
// storage but because maintainer will not install any filter list until
// storage is initialized we can mimic that by returning an empty vector
// from FilteringConfigurationMaintainer::GetCurrentSubscriptions().
TEST_F(AdblockSubscriptionServiceImplTest,
       GetCurrentSubscriptionsWhenMaintainerNotYetInitialized) {
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>());

  ASSERT_EQ(maintainer_factory_calls_.size(), 1u);
  EXPECT_THAT(testee_.GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(
                  maintainer_factory_calls_[0].input_configuration));
  auto tested_entry = std::move(maintainer_factory_calls_[0]);
  // Set subscriptions in prefs.
  auto url1 = GURL{"https://subscription/1.txt"};
  auto url2 = GURL{"https://subscription/2.txt"};
  tested_entry.input_configuration->AddFilterList(url1);
  tested_entry.input_configuration->AddFilterList(url2);
  EXPECT_CALL(*tested_entry.output_maintainer, GetCurrentSubscriptions())
      .WillOnce(Return(std::vector<scoped_refptr<Subscription>>{}));
  auto test_result =
      testee_.GetCurrentSubscriptions(tested_entry.input_configuration);
  EXPECT_EQ(2u, test_result.size());
  for (const GURL& url : {url1, url2}) {
    auto it = base::ranges::find_if(test_result, [&](const auto& entry) {
      return entry->GetSourceUrl() == url;
    });
    ASSERT_TRUE(it != test_result.end());
    EXPECT_TRUE(IsAnEmptySubscription(*it));
  }
}

TEST_F(AdblockSubscriptionServiceImplTest,
       EnabledMaintainersConsultedForSnapshot) {
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("1"));
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("2"));
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("3"));
  ASSERT_EQ(maintainer_factory_calls_.size(), 3u);
  // We disable one configuration. The maintainer of that configuration will
  // not take part in populating the Snapshot because it will be destroyed.
  DisableConfigurationAndEnsureMaintainerDestroyed(
      &maintainer_factory_calls_[1]);
  // The maintainers of enabled configurations will be asked to provide
  // SubscriptionCollections for the Snapshot.
  auto collection1 = std::make_unique<MockSubscriptionCollection>();
  auto collection2 = std::make_unique<MockSubscriptionCollection>();
  const std::vector<MockSubscriptionCollection*> returned_collection_ptrs{
      collection1.get(), collection2.get()};

  EXPECT_CALL(*maintainer_factory_calls_[0].output_maintainer,
              GetSubscriptionCollection())
      .WillOnce(Return(testing::ByMove(std::move(collection1))));
  EXPECT_CALL(*maintainer_factory_calls_[2].output_maintainer,
              GetSubscriptionCollection())
      .WillOnce(Return(testing::ByMove(std::move(collection2))));

  // The SubscriptionCollections that comprise the Snapshot are the ones
  // returned by maintainers.
  const auto snapshot = testee_.GetCurrentSnapshot();
  EXPECT_TRUE(base::ranges::is_permutation(
      snapshot, returned_collection_ptrs, {},
      &std::unique_ptr<SubscriptionCollection>::get));
}

TEST_F(AdblockSubscriptionServiceImplTest,
       SubscriptionObserverNotifiedByMaintainerCallbacks) {
  const GURL kUrl("https://test.com");
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("1"));
  testee_.InstallFilteringConfiguration(
      std::make_unique<FakeFilteringConfiguration>("2"));
  ASSERT_EQ(maintainer_factory_calls_.size(), 2u);

  MockSubscriptionObserver observer;
  testee_.AddObserver(&observer);
  EXPECT_CALL(observer, OnSubscriptionInstalled(kUrl)).Times(2);
  maintainer_factory_calls_[0].input_update_callback.Run(kUrl);
  maintainer_factory_calls_[1].input_update_callback.Run(kUrl);
  testee_.RemoveObserver(&observer);
  // Observer no longer notified after being removed.
  EXPECT_CALL(observer, OnSubscriptionInstalled(kUrl)).Times(0);
  maintainer_factory_calls_[0].input_update_callback.Run(kUrl);
  maintainer_factory_calls_[1].input_update_callback.Run(kUrl);
}

TEST_F(AdblockSubscriptionServiceImplTest, InstallConfigurationWithSameName) {
  auto config1 = std::make_unique<FakeFilteringConfiguration>("test");
  auto* config1_bare_ptr = config1.get();
  testee_.InstallFilteringConfiguration(std::move(config1));
  auto config2 = std::make_unique<FakeFilteringConfiguration>("test");
  testee_.InstallFilteringConfiguration(std::move(config2));
  EXPECT_EQ(testee_.GetInstalledFilteringConfigurations(),
            std::vector<FilteringConfiguration*>{config1_bare_ptr});
}

}  // namespace adblock
