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

#include <list>

#include "base/memory/raw_ptr.h"
#include "base/ranges/algorithm.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "gmock/gmock.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace adblock {

class AdblockFilteringConfigurationBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockFilteringConfigurationBrowserTest() {
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &AdblockFilteringConfigurationBrowserTest::RequestHandler,
        base::Unretained(this)));
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL() == AcceptableAdsUrl()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(
          "[Adblock Plus 2.0]\n"
          "! Checksum: X5A8vtJDBW2a9EgS9glqbg\n"
          "! Version: 202202061935\n"
          "! Last modified: 06 Feb 2022 19:35 UTC\n"
          "! Expires: 1 days (update frequency)\n\n");
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    }
    return nullptr;
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  GURL BlockingFilterListUrl() {
    return embedded_test_server()->GetURL(
        "/filterlist_that_blocks_resource.txt");
  }

  GURL ElementHidingFilterListUrl() {
    return embedded_test_server()->GetURL(
        "/filterlist_that_hides_resource.txt");
  }

  GURL AllowingFilterListUrl() {
    return embedded_test_server()->GetURL(
        "/filterlist_that_allows_resource.txt");
  }

  GURL GetPageUrl() {
    return embedded_test_server()->GetURL("test.org", "/innermost_frame.html");
  }

  void NavigateToPage() {
    ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  }

  std::unique_ptr<PersistentFilteringConfiguration> MakeConfiguration(
      std::string name) {
    return std::make_unique<PersistentFilteringConfiguration>(GetPrefs(),
                                                              std::move(name));
  }

  void InstallFilteringConfiguration(
      std::unique_ptr<FilteringConfiguration> configuration) {
    SubscriptionServiceFactory::GetForBrowserContext(browser_context())
        ->InstallFilteringConfiguration(std::move(configuration));
  }

  void UninstallFilteringConfiguration(const std::string& configuration_name) {
    SubscriptionServiceFactory::GetForBrowserContext(browser_context())
        ->UninstallFilteringConfiguration(configuration_name);
  }

  void WaitUntilSubscriptionsInstalled(std::vector<GURL> subscriptions) {
    auto waiter = GetSubscriptionInstalledWaiter();
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  std::string GetResourcesComputedStyle() {
    const std::string javascript =
        "window.getComputedStyle(document.getElementById('subresource'))."
        "display";
    return content::EvalJs(web_contents(), javascript).ExtractString();
  }

  void ExpectResourceHidden() {
    EXPECT_EQ("none", GetResourcesComputedStyle());
  }

  void ExpectResourceNotHidden() {
    EXPECT_EQ("inline", GetResourcesComputedStyle());
  }

  bool IsResourceLoaded() {
    const std::string javascript =
        "document.getElementById('subresource').naturalHeight !== 0;";
    return content::EvalJs(web_contents(), javascript).ExtractBool();
  }

  void ExpectResourceBlocked() { EXPECT_FALSE(IsResourceLoaded()); }

  void ExpectResourceNotBlocked() { EXPECT_TRUE(IsResourceLoaded()); }
};

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       NoBlockingByDefault) {
  auto configuration = MakeConfiguration("config");
  InstallFilteringConfiguration(std::move(configuration));

  NavigateToPage();
  ExpectResourceNotBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ResourceBlockedByFilteringConfigurationsList) {
  auto configuration = MakeConfiguration("config");
  configuration->AddFilterList(BlockingFilterListUrl());

  InstallFilteringConfiguration(std::move(configuration));

  WaitUntilSubscriptionsInstalled({BlockingFilterListUrl()});

  NavigateToPage();
  ExpectResourceBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ResourceHiddenByFilteringConfigurationsList) {
  auto configuration = MakeConfiguration("config");
  configuration->AddFilterList(ElementHidingFilterListUrl());

  InstallFilteringConfiguration(std::move(configuration));

  WaitUntilSubscriptionsInstalled({ElementHidingFilterListUrl()});

  NavigateToPage();
  ExpectResourceHidden();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ResourceAllowedByFilteringConfigurationsList) {
  auto configuration = MakeConfiguration("config");
  configuration->AddFilterList(BlockingFilterListUrl());
  configuration->AddFilterList(ElementHidingFilterListUrl());
  configuration->AddFilterList(AllowingFilterListUrl());

  InstallFilteringConfiguration(std::move(configuration));

  WaitUntilSubscriptionsInstalled({BlockingFilterListUrl(),
                                   AllowingFilterListUrl(),
                                   ElementHidingFilterListUrl()});

  NavigateToPage();
  ExpectResourceNotBlocked();
  ExpectResourceNotHidden();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       BlockingTakesPrecedenceBetweenConfigurations) {
  auto blocking_configuration = MakeConfiguration("blocking");
  blocking_configuration->AddFilterList(BlockingFilterListUrl());

  auto allowing_configuration = MakeConfiguration("allowing");
  allowing_configuration->AddFilterList(AllowingFilterListUrl());

  InstallFilteringConfiguration(std::move(blocking_configuration));
  InstallFilteringConfiguration(std::move(allowing_configuration));

  WaitUntilSubscriptionsInstalled(
      {BlockingFilterListUrl(), AllowingFilterListUrl()});

  NavigateToPage();
  ExpectResourceBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ElementBlockedByCustomFilter) {
  auto configuration = MakeConfiguration("config");
  configuration->AddCustomFilter("*resource.png");

  InstallFilteringConfiguration(std::move(configuration));

  NavigateToPage();
  ExpectResourceBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ElementAllowedByCustomFilter) {
  auto configuration = MakeConfiguration("config");
  configuration->AddCustomFilter("*resource.png");
  configuration->AddCustomFilter("@@*resource.png");

  InstallFilteringConfiguration(std::move(configuration));

  NavigateToPage();
  ExpectResourceNotBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ElementAllowedByAllowedDomain) {
  auto configuration = MakeConfiguration("config");
  configuration->AddCustomFilter("*resource.png");
  configuration->AddAllowedDomain(GetPageUrl().host());

  InstallFilteringConfiguration(std::move(configuration));

  NavigateToPage();
  ExpectResourceNotBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_CustomFiltersPersist) {
  auto configuration = MakeConfiguration("persistent");
  // This custom filter will survive browser restart.
  configuration->AddCustomFilter("*resource.png");
  InstallFilteringConfiguration(std::move(configuration));
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       CustomFiltersPersist) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  EXPECT_THAT((*configuration)->GetCustomFilters(),
              testing::UnorderedElementsAre("*resource.png"));

  NavigateToPage();
  ExpectResourceBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       DisabledConfigurationDoesNotBlock) {
  auto configuration = MakeConfiguration("config");
  configuration->AddCustomFilter("*resource.png");
  configuration->SetEnabled(false);

  InstallFilteringConfiguration(std::move(configuration));

  NavigateToPage();
  ExpectResourceNotBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ConfigurationCanBeUsedAfterInstalling) {
  auto configuration = MakeConfiguration("config");
  auto* configuration_ptr = configuration.get();

  InstallFilteringConfiguration(std::move(configuration));

  configuration_ptr->AddCustomFilter("*resource.png");

  NavigateToPage();
  ExpectResourceBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       ConfigurationCanBeDisabledAfterInstalling) {
  auto configuration = MakeConfiguration("config");
  auto* configuration_ptr = configuration.get();

  InstallFilteringConfiguration(std::move(configuration));

  configuration_ptr->AddCustomFilter("*resource.png");
  configuration_ptr->SetEnabled(false);

  NavigateToPage();
  ExpectResourceNotBlocked();
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       SubscriptionsDownloadedAfterConfigurationEnabled) {
  auto configuration = MakeConfiguration("config");
  configuration->SetEnabled(false);
  configuration->AddFilterList(BlockingFilterListUrl());
  configuration->AddFilterList(ElementHidingFilterListUrl());
  configuration->AddFilterList(AllowingFilterListUrl());
  auto* configuration_ptr = configuration.get();

  InstallFilteringConfiguration(std::move(configuration));

  configuration_ptr->SetEnabled(true);

  WaitUntilSubscriptionsInstalled({BlockingFilterListUrl(),
                                   AllowingFilterListUrl(),
                                   ElementHidingFilterListUrl()});
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_DownloadedSubscriptionsPersistOnDisk) {
  auto configuration = MakeConfiguration("config");
  // This filter list setting will survive browser restart.
  configuration->AddFilterList(BlockingFilterListUrl());

  InstallFilteringConfiguration(std::move(configuration));

  // This downloaded subscription won't need to be re-downloaded after restart.
  WaitUntilSubscriptionsInstalled({BlockingFilterListUrl()});
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       DownloadedSubscriptionsPersistOnDisk) {
  NavigateToPage();
  ExpectResourceBlocked();
}

// 1st run: create.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_PRE_CreateThenRemoveCustomConfiguration) {
  auto configuration = MakeConfiguration("persistent");
  InstallFilteringConfiguration(std::move(configuration));
}

// 2nd run: remove.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_CreateThenRemoveCustomConfiguration) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  UninstallFilteringConfiguration("persistent");
  configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  configuration = base::ranges::find(configurations, "persistent",
                                     &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration == configurations.end());
}

// 3rd run: verify not present.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       CreateThenRemoveCustomConfiguration) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration == configurations.end());
}

// 1st run: confirm "adblock" configuration is created and contains expected
// default settings, then change some settings and verify in the next run.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_PRE_RemoveAdblockConfiguration) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  ASSERT_TRUE((*configuration)->IsEnabled());
  auto domains = (*configuration)->GetAllowedDomains();
  ASSERT_TRUE(domains.empty());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_EQ(3u, subscriptions.size());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        DefaultSubscriptionUrl()) != subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AntiCVUrl()) != subscriptions.end());
  // Change some settings and check in 2nd run.
  (*configuration)->AddAllowedDomain("example.com");
  (*configuration)->SetEnabled(false);
}

// 2nd run: make sure that previously changed settings are persisted, then
// remove "adblock" configuration.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_RemoveAdblockConfiguration) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());

  // Check previously changed settings.
  ASSERT_FALSE((*configuration)->IsEnabled());
  auto domains = (*configuration)->GetAllowedDomains();
  ASSERT_EQ(1u, domains.size());
  ASSERT_EQ("example.com", (*configuration)->GetAllowedDomains().front());

  UninstallFilteringConfiguration(kAdblockFilteringConfigurationName);
  configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration == configurations.end());
}

// 3rd run: verify "adblock" configuration is not present.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       RemoveAdblockConfiguration) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration == configurations.end());
}

// 1st run: set legacy prefs and verify migration in the next run.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_MigrateSettings) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto* prefs = GetPrefs();
  ASSERT_FALSE(
      prefs->GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  ASSERT_TRUE(
      prefs->GetList(common::prefs::kAdblockCustomSubscriptionsLegacy).empty());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        GURL{"https://custom.bar"}) == subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        GURL{"https://default.bar"}) == subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
  ASSERT_TRUE((*configuration)->GetAllowedDomains().empty());
  ASSERT_TRUE((*configuration)->GetCustomFilters().empty());
  ASSERT_TRUE((*configuration)->IsEnabled());

  // Now set legacy prefs.
  {
    ScopedListPrefUpdate update(
        prefs, common::prefs::kAdblockCustomSubscriptionsLegacy);
    update->Append("https://custom.bar");
  }
  {
    ScopedListPrefUpdate update(prefs,
                                common::prefs::kAdblockSubscriptionsLegacy);
    update->Append("https://default.bar");
  }
  {
    ScopedListPrefUpdate update(prefs,
                                common::prefs::kAdblockAllowedDomainsLegacy);
    update->Append("example.com");
  }
  {
    ScopedListPrefUpdate update(prefs,
                                common::prefs::kAdblockCustomFiltersLegacy);
    update->Append("test.com$script");
  }
  prefs->SetBoolean(common::prefs::kEnableAdblockLegacy, false);
  prefs->SetBoolean(common::prefs::kEnableAcceptableAdsLegacy, false);

  // Remove "adblock" configuration to trigger migration in the next run.
  UninstallFilteringConfiguration(kAdblockFilteringConfigurationName);
}

// 2nd run: check migrated settings.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       MigrateSettings) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto* prefs = GetPrefs();
  ASSERT_FALSE(
      prefs->GetBoolean(common::prefs::kInstallFirstStartSubscriptions));
  ASSERT_FALSE((*configuration)->IsEnabled());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        GURL{"https://custom.bar"}) != subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        GURL{"https://default.bar"}) != subscriptions.end());
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) == subscriptions.end());
  auto domains = (*configuration)->GetAllowedDomains();
  ASSERT_TRUE(std::find(domains.begin(), domains.end(), "example.com") !=
              domains.end());
  auto filters = (*configuration)->GetCustomFilters();
  ASSERT_TRUE(std::find(filters.begin(), filters.end(), "test.com$script") !=
              filters.end());
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_PersistDisabledAAState) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
  (*configuration)->RemoveFilterList(AcceptableAdsUrl());
  subscriptions = (*configuration)->GetFilterLists();
  ASSERT_FALSE(std::find(subscriptions.begin(), subscriptions.end(),
                         AcceptableAdsUrl()) != subscriptions.end());
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PersistDisabledAAState) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_FALSE(std::find(subscriptions.begin(), subscriptions.end(),
                         AcceptableAdsUrl()) != subscriptions.end());
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PRE_PersistEnabledAAState) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
  (*configuration)->RemoveFilterList(AcceptableAdsUrl());
  subscriptions = (*configuration)->GetFilterLists();
  ASSERT_FALSE(std::find(subscriptions.begin(), subscriptions.end(),
                         AcceptableAdsUrl()) != subscriptions.end());
  (*configuration)->AddFilterList(AcceptableAdsUrl());
  subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
}

IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationBrowserTest,
                       PersistEnabledAAState) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration =
      base::ranges::find(configurations, kAdblockFilteringConfigurationName,
                         &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  auto subscriptions = (*configuration)->GetFilterLists();
  ASSERT_TRUE(std::find(subscriptions.begin(), subscriptions.end(),
                        AcceptableAdsUrl()) != subscriptions.end());
}

class AdblockFilteringConfigurationDisableSwitchBrowserTest
    : public AdblockFilteringConfigurationBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (base::StartsWith(
            ::testing::UnitTest::GetInstance()->current_test_info()->name(),
            "PRE_CreateConfigAndConfirmEnableStateAfterReset")) {
      command_line->AppendSwitch(adblock::switches::kDisableEyeoFiltering);
    }
  }
};

// 1st run: create configuration and make sure it is enabled by default.
IN_PROC_BROWSER_TEST_F(
    AdblockFilteringConfigurationDisableSwitchBrowserTest,
    PRE_PRE_PRE_CreateConfigAndConfirmEnableStateAfterReset) {
  auto configuration = MakeConfiguration("persistent");
  ASSERT_TRUE(configuration->IsEnabled());
  InstallFilteringConfiguration(std::move(configuration));
}

// 2nd run: make sure configuration is enabled after restart.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationDisableSwitchBrowserTest,
                       PRE_PRE_CreateConfigAndConfirmEnableStateAfterReset) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  ASSERT_TRUE((*configuration)->IsEnabled());
}

// 3rd run: after adding "--disable-eyeo-filtering" make sure configuration is
// disabled.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationDisableSwitchBrowserTest,
                       PRE_CreateConfigAndConfirmEnableStateAfterReset) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  ASSERT_FALSE((*configuration)->IsEnabled());
}

// 4th run: without "--disable-eyeo-filtering" make sure configuration is still
// disabled.
IN_PROC_BROWSER_TEST_F(AdblockFilteringConfigurationDisableSwitchBrowserTest,
                       CreateConfigAndConfirmEnableStateAfterReset) {
  auto configurations =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetInstalledFilteringConfigurations();
  auto configuration = base::ranges::find(configurations, "persistent",
                                          &FilteringConfiguration::GetName);
  ASSERT_TRUE(configuration != configurations.end());
  ASSERT_FALSE((*configuration)->IsEnabled());
}

}  // namespace adblock
