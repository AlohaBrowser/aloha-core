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

#include "components/adblock/content/browser/factories/subscription_service_factory.h"

#include "base/command_line.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

namespace {
bool IsFilterListPresent(const std::vector<GURL>& subscriptions,
                         const GURL& url) {
  return std::find(subscriptions.begin(), subscriptions.end(), url) !=
         subscriptions.end();
}
}  // namespace

class SubscriptionServiceFactoryTestImpl : public SubscriptionServiceFactory {
 public:
  SubscriptionServiceFactoryTestImpl() = default;
  ~SubscriptionServiceFactoryTestImpl() override = default;

  SubscriptionService* GetSubscriptionService(
      content::BrowserContext* context) {
    auto uptr = BuildServiceInstanceForBrowserContext(context);
    return static_cast<SubscriptionService*>(uptr.release());
  }

  void SetPrefs(TestingPrefServiceSimple* prefs) {
    prefs_ = prefs;
    adblock::common::prefs::RegisterProfilePrefs(prefs_->registry());
  }

  void SetLocale(std::string locale) { locale_ = locale; }

 protected:
  SubscriptionPersistentMetadata* GetSubscriptionPersistentMetadata(
      content::BrowserContext* context) const override {
    static MockSubscriptionPersistentMetadata
        mock_subscription_persistent_metadata;
    return &mock_subscription_persistent_metadata;
  }

  PrefService* GetPrefs(content::BrowserContext* context) const override {
    return prefs_;
  }

  std::string GetLocale() const override { return locale_; }

 private:
  std::string locale_ = "en-US";
  raw_ptr<TestingPrefServiceSimple> prefs_;
};

class SubscriptionServiceFactoryTest : public testing::Test {
 public:
  SubscriptionServiceFactoryTest() = default;

  void SetUp() override {
    subscription_service_factory_test_impl_.SetPrefs(&prefs_);
  }

  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
  TestingPrefServiceSimple prefs_;
  SubscriptionServiceFactoryTestImpl subscription_service_factory_test_impl_;
};

TEST_F(SubscriptionServiceFactoryTest, TestFirstRun) {
  ASSERT_TRUE(
      prefs_.GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    // Confirm 1st run flag is now off and default settings are applied.
    ASSERT_FALSE(
        prefs_.GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
    ASSERT_TRUE(configuration->IsEnabled());
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_EQ(3u, subscriptions.size());
    ASSERT_TRUE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
    ASSERT_TRUE(IsFilterListPresent(subscriptions, DefaultSubscriptionUrl()));
    ASSERT_TRUE(IsFilterListPresent(subscriptions, AntiCVUrl()));

    // Change settings.
    configuration->SetEnabled(false);
    configuration->RemoveFilterList(AcceptableAdsUrl());
    configuration->RemoveFilterList(DefaultSubscriptionUrl());
    configuration->RemoveFilterList(AntiCVUrl());
  }

  {
    // Recreate service and make sure settings are not overwritten.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(
        subscription_service
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName)
            ->IsEnabled());
    ASSERT_TRUE(configuration->GetFilterLists().empty());
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestFirstRunNonEnglishLocale) {
  auto pl_and_en_subscription = GURL{
      "https://easylist-downloads.adblockplus.org/"
      "easylistpolish+easylist.txt"};
  auto pl_subscription = GURL{
      "https://easylist-downloads.adblockplus.org/"
      "easylistpolish.txt"};
  subscription_service_factory_test_impl_.SetLocale("pl-PL");
  ASSERT_TRUE(
      prefs_.GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  std::unique_ptr<SubscriptionService> subscription_service(
      subscription_service_factory_test_impl_.GetSubscriptionService(
          &browser_context_));
  auto* configuration = subscription_service->GetFilteringConfiguration(
      kAdblockFilteringConfigurationName);
  // Confirm 1st run flag is now off and default settings are applied.
  ASSERT_FALSE(
      prefs_.GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  ASSERT_TRUE(configuration->IsEnabled());
  auto subscriptions = configuration->GetFilterLists();
  ASSERT_EQ(4u, subscriptions.size());
  ASSERT_TRUE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
  ASSERT_TRUE(IsFilterListPresent(subscriptions, AntiCVUrl()));
  ASSERT_FALSE(IsFilterListPresent(subscriptions, pl_and_en_subscription));
  ASSERT_TRUE(IsFilterListPresent(subscriptions, DefaultSubscriptionUrl()));
  ASSERT_TRUE(IsFilterListPresent(subscriptions, pl_subscription));
}

TEST_F(SubscriptionServiceFactoryTest, TestDisableAcceptableAdsSwitch) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kDisableAcceptableAds);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_FALSE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));

    // Re-enable AA.
    configuration->AddFilterList(AcceptableAdsUrl());
  }

  {
    // Verify switch is applied again.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_FALSE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestDisableAdblockSwitch) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kDisableAdblock);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsEnabled());

    // Re-enable.
    configuration->SetEnabled(true);
  }

  {
    // Verify switch is applied again.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsEnabled());
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestDisableEyeoFilteringSwitch) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kDisableEyeoFiltering);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsEnabled());

    // Re-enable.
    configuration->SetEnabled(true);
  }

  {
    // Verify switch is applied again.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsEnabled());
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestDisableEyeoByDefault) {
  auto flag_set = OverrideEyeoFilteringDisabledByDefault(true);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsEnabled());

    // Re-enable.
    configuration->SetEnabled(true);
  }

  {
    // Verify 1st run switch is NOT applied again.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_TRUE(configuration->IsEnabled());
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestDisableAAByDefault) {
  auto flag_set = OverrideAcceptableAdsDisabledByDefault(true);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_FALSE(configuration->IsFilterListPresent(AcceptableAdsUrl()));

    // Re-enable.
    configuration->AddFilterList(AcceptableAdsUrl());
  }

  {
    // Verify 1st run switch is NOT applied again.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    ASSERT_TRUE(configuration->IsFilterListPresent(AcceptableAdsUrl()));
  }
}

TEST_F(SubscriptionServiceFactoryTest, TestSettingsMigration) {
  prefs_.SetBoolean(common::prefs::kInstallFirstStartSubscriptions, false);
  {
    // Now set legacy prefs.
    {
      ScopedListPrefUpdate update(&prefs_,
                                  common::prefs::kAdblockSubscriptionsLegacy);
      update->Append("https://default.bar");
    }

    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    // Verify settings were migrated.
    ASSERT_TRUE(configuration->IsEnabled());
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_EQ(2u, subscriptions.size());
    ASSERT_TRUE(
        IsFilterListPresent(subscriptions, GURL{"https://default.bar"}));
    ASSERT_TRUE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
    // Disable AA.
    configuration->RemoveFilterList(AcceptableAdsUrl());
  }
  {
    // Verify that AA is not migrated 2nd time.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_EQ(1u, subscriptions.size());
    ASSERT_TRUE(
        IsFilterListPresent(subscriptions, GURL{"https://default.bar"}));
    ASSERT_FALSE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
  }
}

void VerifyCombinedFilterListsMigratedToStandalone(
    const std::unique_ptr<SubscriptionService>& subscription_service) {
  auto* configuration = subscription_service->GetFilteringConfiguration(
      kAdblockFilteringConfigurationName);
  auto subscriptions = configuration->GetFilterLists();
  ASSERT_TRUE(IsFilterListPresent(subscriptions,
                                  GURL{"https://default.bar/file3+file4.txt"}));
  ASSERT_TRUE(IsFilterListPresent(
      subscriptions, GURL{AdblockBaseFilterListUrl().spec() + "file.txt"}));
  ASSERT_TRUE(IsFilterListPresent(
      subscriptions,
      GURL{AdblockBaseFilterListUrl().spec() + "file1+file2.txt"}));
  ASSERT_FALSE(IsFilterListPresent(
      subscriptions, GURL{AdblockBaseFilterListUrl().spec() + "file1.txt"}));
  ASSERT_FALSE(IsFilterListPresent(
      subscriptions, GURL{AdblockBaseFilterListUrl().spec() + "file2.txt"}));
  ASSERT_FALSE(IsFilterListPresent(
      subscriptions,
      GURL{AdblockBaseFilterListUrl().spec() + "abpindo+easylist.txt"}));
  ASSERT_TRUE(IsFilterListPresent(
      subscriptions, GURL{AdblockBaseFilterListUrl().spec() + "abpindo.txt"}));
  ASSERT_TRUE(IsFilterListPresent(
      subscriptions, GURL{AdblockBaseFilterListUrl().spec() + "easylist.txt"}));
}

TEST_F(SubscriptionServiceFactoryTest,
       TestSettingsMigrationToGeolocationFromOldPrefs) {
  prefs_.SetBoolean(common::prefs::kInstallFirstStartSubscriptions, false);
  // Now set legacy prefs.
  {
    ScopedListPrefUpdate subscriptions(
        &prefs_, common::prefs::kAdblockSubscriptionsLegacy);
    subscriptions->Append(AdblockBaseFilterListUrl().spec() + "file.txt");
    subscriptions->Append(AdblockBaseFilterListUrl().spec() +
                          "abpindo+easylist.txt");
    // Should not be migrated
    subscriptions->Append(AdblockBaseFilterListUrl().spec() +
                          "file1+file2.txt");
    subscriptions->Append("https://default.bar/file3+file4.txt");
  }
  std::unique_ptr<SubscriptionService> subscription_service(
      subscription_service_factory_test_impl_.GetSubscriptionService(
          &browser_context_));
  VerifyCombinedFilterListsMigratedToStandalone(subscription_service);
}

TEST_F(SubscriptionServiceFactoryTest,
       TestSettingsMigrationToGeolocationFromNewPrefs) {
  prefs_.SetBoolean(common::prefs::kInstallFirstStartSubscriptions, false);
  // Now set "adblock" configuration prefs.
  {
    base::Value::Dict adblock_data;
    adblock_data.EnsureList("subscriptions");
    auto* subscriptions = adblock_data.FindList("subscriptions");
    subscriptions->Append(AdblockBaseFilterListUrl().spec() + "file.txt");
    subscriptions->Append(AdblockBaseFilterListUrl().spec() +
                          "abpindo+easylist.txt");
    // Should not be migrated
    subscriptions->Append(AdblockBaseFilterListUrl().spec() +
                          "file1+file2.txt");
    subscriptions->Append("https://default.bar/file3+file4.txt");
    ScopedDictPrefUpdate update(&prefs_,
                                common::prefs::kConfigurationsPrefsPath);
    update.Get().Set(kAdblockFilteringConfigurationName, adblock_data.Clone());
  }
  std::unique_ptr<SubscriptionService> subscription_service(
      subscription_service_factory_test_impl_.GetSubscriptionService(
          &browser_context_));
  VerifyCombinedFilterListsMigratedToStandalone(subscription_service);
}

TEST_F(SubscriptionServiceFactoryTest, TestSettingsMigrationWithFirstRun) {
  ASSERT_TRUE(
      prefs_.GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  // Now set legacy prefs.
  {
    ScopedListPrefUpdate update(
        &prefs_, common::prefs::kAdblockCustomSubscriptionsLegacy);
    update->Append("https://custom.bar");
  }
  {
    ScopedListPrefUpdate update(&prefs_,
                                common::prefs::kAdblockSubscriptionsLegacy);
    update->Append("https://default.bar");
  }
  {
    ScopedListPrefUpdate update(&prefs_,
                                common::prefs::kAdblockAllowedDomainsLegacy);
    update->Append("example.com");
  }
  {
    ScopedListPrefUpdate update(&prefs_,
                                common::prefs::kAdblockCustomFiltersLegacy);
    update->Append("test.com$script");
  }
  prefs_.SetBoolean(common::prefs::kEnableAdblockLegacy, false);
  prefs_.SetBoolean(common::prefs::kEnableAcceptableAdsLegacy, false);

  {
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    // Verify settings were migrated.
    ASSERT_FALSE(configuration->IsEnabled());
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_TRUE(IsFilterListPresent(subscriptions, GURL{"https://custom.bar"}));
    ASSERT_TRUE(
        IsFilterListPresent(subscriptions, GURL{"https://default.bar"}));
    ASSERT_FALSE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
    // Along with migration default subscriptions were applied (1st run logic).
    ASSERT_TRUE(IsFilterListPresent(subscriptions, DefaultSubscriptionUrl()));
    ASSERT_TRUE(IsFilterListPresent(subscriptions, AntiCVUrl()));
    auto domains = configuration->GetAllowedDomains();
    ASSERT_TRUE(std::find(domains.begin(), domains.end(), "example.com") !=
                domains.end());
    auto filters = configuration->GetCustomFilters();
    ASSERT_TRUE(std::find(filters.begin(), filters.end(), "test.com$script") !=
                filters.end());

    // Change some settings.
    configuration->SetEnabled(false);
    configuration->RemoveFilterList(GURL{"https://custom.bar"});
    configuration->RemoveAllowedDomain("example.com");
  }

  {
    // Recreate service and make sure settings are not overwritten.
    std::unique_ptr<SubscriptionService> subscription_service(
        subscription_service_factory_test_impl_.GetSubscriptionService(
            &browser_context_));
    auto* configuration = subscription_service->GetFilteringConfiguration(
        kAdblockFilteringConfigurationName);
    auto subscriptions = configuration->GetFilterLists();
    ASSERT_FALSE(
        IsFilterListPresent(subscriptions, GURL{"https://custom.bar"}));
    ASSERT_TRUE(
        IsFilterListPresent(subscriptions, GURL{"https://default.bar"}));
    ASSERT_FALSE(IsFilterListPresent(subscriptions, AcceptableAdsUrl()));
    auto domains = configuration->GetAllowedDomains();
    ASSERT_TRUE(domains.empty());
    auto filters = configuration->GetCustomFilters();
    ASSERT_TRUE(std::find(filters.begin(), filters.end(), "test.com$script") !=
                filters.end());
  }
}

}  // namespace adblock
