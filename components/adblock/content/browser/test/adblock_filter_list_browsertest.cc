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

#include "base/check.h"
#include "base/environment.h"
#include "base/functional/callback_forward.h"
#include "base/strings/stringprintf.h"
#include "base/time/time_override.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/subscription/recommended_subscription_installer_impl.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata_impl.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/version_info/version_info.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

using base::subtle::TimeNowIgnoringOverride;

class AdblockFilterListDownloadTestBase : public AdblockBrowserTestBase {
 public:
  AdblockFilterListDownloadTestBase()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&AdblockFilterListDownloadTestBase::RequestHandler,
                            base::Unretained(this)));
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org"};
    https_server_.SetSSLConfig(cert_config);
    EXPECT_TRUE(https_server_.Start());
    SetFilterListServerPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("easylist-downloads.adblockplus.org", "127.0.0.1");
  }

  void CheckRequestParams(const net::test_server::HttpRequest& request,
                          std::string expected_disabled_value) {
    std::string os;
    base::ReplaceChars(version_info::GetOSType(), base::kWhitespaceASCII, "",
                       &os);
    EXPECT_TRUE(request.relative_url.find("addonName=eyeo-chromium-sdk") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("addonVersion=2.0.0") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("platformVersion=1.0") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("platform=" + os) !=
                std::string::npos);
    if (RunsOnEyeoCI()) {
      // Those two checks below require "eyeo_application_name" and
      // "eyeo_application_version" to be set as gn gen args.
      EXPECT_TRUE(
          request.relative_url.find("application=app_name_from_ci_config") !=
          std::string::npos)
          << "Did you set \"eyeo_application_name\" gn gen arg?";
      EXPECT_TRUE(request.relative_url.find(
                      "applicationVersion=app_version_from_ci_config") !=
                  std::string::npos)
          << "Did you set \"eyeo_application_version\" gn gen arg?";
    }
    EXPECT_TRUE(
        request.relative_url.find("disabled=" + expected_disabled_value) !=
        std::string::npos);
    EXPECT_TRUE(request.relative_url.find("safe=true") != std::string::npos);
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(request.relative_url, "/easylist.txt") ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt"))) {
      CheckRequestParams(request, "false");
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    // This is fine for the purpose of this test.
    return nullptr;
  }

  bool RunsOnEyeoCI() {
    auto env = base::Environment::Create();
    std::string value;
    env->GetVar("CI_PROJECT_NAME", &value);
    return value == "chromium-sdk";
  }

 protected:
  net::EmbeddedTestServer https_server_;
  std::set<std::string> default_lists_;
  bool finish_condition_met_ = false;
  base::RepeatingClosure quit_closure_;
};

class AdblockEnabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase {
 public:
  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    auto result = AdblockFilterListDownloadTestBase::RequestHandler(request);
    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (CheckExpectedDownloads()) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return result;
  }

  bool CheckExpectedDownloads() {
    return (default_lists_.find("abp-filters-anti-cv.txt") !=
            default_lists_.end()) &&
           (default_lists_.find("easylist.txt") != default_lists_.end()) &&
           (default_lists_.find("exceptionrules.txt") != default_lists_.end());
  }
};

IN_PROC_BROWSER_TEST_F(AdblockEnabledFilterListDownloadTest,
                       TestInitialDownloads) {
  RunUntilTestFinished();
}

class AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase {
 public:
  AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest() {
    const auto testing_interval = base::Seconds(1);
    SubscriptionServiceFactory::SetUpdateCheckIntervalForTesting(
        testing_interval);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    // If we get expected HEAD request we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (request.method == net::test_server::HttpMethod::METHOD_HEAD &&
        base::StartsWith(request.relative_url, "/exceptionrules.txt")) {
      CheckRequestParams(request, "true");
      NotifyTestFinished();
    }

    return nullptr;
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(adblock::switches::kDisableAcceptableAds);
  }
};

IN_PROC_BROWSER_TEST_F(
    AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest,
    TestInitialDownloads) {
  RunUntilTestFinished();
}

enum class DisableSwitch { Adblock, Eyeo };

class AdblockDisabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase,
      public testing::WithParamInterface<DisableSwitch> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(GetParam() == DisableSwitch::Adblock
                                   ? adblock::switches::kDisableAdblock
                                   : adblock::switches::kDisableEyeoFiltering);
  }

  void VerifyNoDownloads() {
    ASSERT_EQ(0u, default_lists_.size());
    NotifyTestFinished();
  }
};

IN_PROC_BROWSER_TEST_P(AdblockDisabledFilterListDownloadTest,
                       TestInitialDownloads) {
  // This test assumes that inital downloads (for adblock enabled) will happen
  // within 10 seconds. When tested locally it always happens within 3
  // seconds.
  base::OneShotTimer timer;
  timer.Start(
      FROM_HERE, base::Seconds(10),
      base::BindOnce(&AdblockDisabledFilterListDownloadTest::VerifyNoDownloads,
                     base::Unretained(this)));
  RunUntilTestFinished();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockDisabledFilterListDownloadTest,
                         testing::Values(DisableSwitch::Adblock,
                                         DisableSwitch::Eyeo));

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

enum class Country {
  Arabic,
  Bulgaria,
  China,
  Czech,
  France,
  Germany,
  Hungary,
  India,
  Indonesia,
  Israel,
  Italy,
  Japan,
  Korea,
  Latvia,
  Lithuania,
  Netherlands,
  Norway,
  Poland,
  Portugal,
  Romania,
  Russia,
  Spain,
  Thailand,
  Turkey,
  Vietnam
};

class AdblockLocaleFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase,
      public testing::WithParamInterface<Country> {
 public:
  using LocaleToEasylistPath = std::pair<std::string, std::vector<std::string>>;
  AdblockLocaleFilterListDownloadTest() : locale_data_(GetLocale(GetParam())) {
    setenv("LANGUAGE", locale_data_.first.c_str(), 1);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt") ||
         base::StartsWith(request.relative_url, "/easylist.txt") ||
         IsExpectedCountrySpecificPath(request.relative_url, locale_data_))) {
      CheckRequestParams(request, "false");
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }

    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (default_lists_.size() == (3 + locale_data_.second.size())) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

 private:
  LocaleToEasylistPath locale_data_;
  LocaleToEasylistPath GetLocale(Country country) {
    switch (country) {
      case Country::Arabic:
        return std::make_pair("ar_SA", std::vector<std::string>{
                                           "/liste_ar.txt", "/liste_fr.txt"});
      case Country::Bulgaria:
        return std::make_pair("bg_BG",
                              std::vector<std::string>{"/bulgarian_list.txt"});
      case Country::China:
        return std::make_pair("zh_CN",
                              std::vector<std::string>{"/easylistchina.txt"});
      case Country::Czech:
        return std::make_pair(
            "cs_CZ", std::vector<std::string>{"/easylistczechslovak.txt"});
      case Country::France:
        return std::make_pair("fr_FR",
                              std::vector<std::string>{"/liste_fr.txt"});
      case Country::Germany:
        return std::make_pair("de_DE",
                              std::vector<std::string>{"/easylistgermany.txt"});
      case Country::Hungary:
        return std::make_pair("hu_HU",
                              std::vector<std::string>{"/hufilter.txt"});
      case Country::India:
        return std::make_pair("ml_IN",
                              std::vector<std::string>{"/indianlist.txt"});
      case Country::Indonesia:
        return std::make_pair("id_ID",
                              std::vector<std::string>{"/abpindo.txt"});
      case Country::Israel:
        return std::make_pair("he_IL",
                              std::vector<std::string>{"/israellist.txt"});
      case Country::Italy:
        return std::make_pair("it_IT",
                              std::vector<std::string>{"/easylistitaly.txt"});
      case Country::Japan:
        return std::make_pair(
            "ja_JP", std::vector<std::string>{"/japanese-filters.txt"});
      case Country::Korea:
        return std::make_pair("ko_KR",
                              std::vector<std::string>{"/koreanlist.txt"});
      case Country::Latvia:
        return std::make_pair("lv_LV",
                              std::vector<std::string>{"/latvianlist.txt"});
      case Country::Lithuania:
        return std::make_pair(
            "lt_LT", std::vector<std::string>{"/easylistlithuania.txt"});
      case Country::Netherlands:
        return std::make_pair("nl_NL",
                              std::vector<std::string>{"/easylistdutch.txt"});
      case Country::Norway:
        return std::make_pair(
            "no_NO",
            std::vector<std::string>{"/dandelion_sprouts_nordic_filters.txt"});
      case Country::Poland:
        return std::make_pair("pl_PL",
                              std::vector<std::string>{"/easylistpolish.txt"});
      case Country::Portugal:
        return std::make_pair(
            "pt_PT", std::vector<std::string>{"/easylistportuguese.txt"});
      case Country::Romania:
        return std::make_pair("ro_RO", std::vector<std::string>{"/rolist.txt"});
      case Country::Russia:
        return std::make_pair("ru_RU",
                              std::vector<std::string>{"/ruadlist.txt"});
      case Country::Spain:
        return std::make_pair("es_ES",
                              std::vector<std::string>{"/easylistspanish.txt"});
      case Country::Thailand:
        return std::make_pair("th_TH",
                              std::vector<std::string>{"/global-filters.txt"});
      case Country::Turkey:
        return std::make_pair("tr_TR",
                              std::vector<std::string>{"/turkish-filters.txt"});
      case Country::Vietnam:
        return std::make_pair("vi_VN", std::vector<std::string>{"/abpvn.txt"});
      default:
        return std::make_pair("en_US",
                              std::vector<std::string>{"/easylist.txt"});
    }
  }

  bool IsExpectedCountrySpecificPath(const std::string& expected_path,
                                     const LocaleToEasylistPath& locale_data) {
    auto language = locale_data.first.substr(0, 2);
    return std::ranges::any_of(
        config::GetKnownSubscriptions(), [&](const auto& config_entry) {
          return std::ranges::count(config_entry.languages, language) > 0 &&
                 base::StartsWith(expected_path, config_entry.url.path());
        });
  }
};

IN_PROC_BROWSER_TEST_P(AdblockLocaleFilterListDownloadTest,
                       TestInitialDownloads) {
  RunUntilTestFinished();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockLocaleFilterListDownloadTest,
                         testing::Values(Country::Arabic,
                                         Country::Bulgaria,
                                         Country::China,
                                         Country::Czech,
                                         Country::France,
                                         Country::Germany,
                                         Country::Hungary,
                                         Country::India,
                                         Country::Indonesia,
                                         Country::Israel,
                                         Country::Italy,
                                         Country::Japan,
                                         Country::Korea,
                                         Country::Latvia,
                                         Country::Lithuania,
                                         Country::Netherlands,
                                         Country::Norway,
                                         Country::Poland,
                                         Country::Portugal,
                                         Country::Romania,
                                         Country::Russia,
                                         Country::Spain,
                                         Country::Thailand,
                                         Country::Turkey,
                                         Country::Vietnam));

#endif

class AdblockCombinedFilterListMigratedTest
    : public AdblockFilterListDownloadTestBase {
 public:
  AdblockCombinedFilterListMigratedTest() {
    SubscriptionServiceFactory::SetUpdateCheckIntervalForTesting(
        base::Seconds(1));
  }
  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    EXPECT_FALSE(
        base::StartsWith(request.relative_url, "/easylistpolish+easylist.txt"));
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/easylist.txt") ||
         base::StartsWith(request.relative_url, "/easylistpolish.txt"))) {
      CheckRequestParams(request, "false");
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }

    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (default_lists_.size() == 2) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(adblock::switches::kDisableAdblock);
  }
};

IN_PROC_BROWSER_TEST_F(AdblockCombinedFilterListMigratedTest,
                       PRE_TestMigration) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  for (const auto& url : adblock_configuration->GetFilterLists()) {
    adblock_configuration->RemoveFilterList(url);
  }
  adblock_configuration->AddFilterList(
      GURL{AdblockBaseFilterListUrl().spec() + "easylistpolish+easylist.txt"});
}

IN_PROC_BROWSER_TEST_F(AdblockCombinedFilterListMigratedTest, TestMigration) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  adblock_configuration->SetEnabled(true);
  RunUntilTestFinished();
}

class AdblockGeolocatedFilterListsTest
    : public AdblockFilterListDownloadTestBase,
      public adblock::FilteringConfiguration::Observer {
 public:
  AdblockGeolocatedFilterListsTest() {
    const auto testing_interval = base::Seconds(1);
    SubscriptionServiceFactory::SetUpdateCheckIntervalForTesting(
        testing_interval);
    list_pl_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/easylistpolish.txt",
        https_server_.port());
    list_de_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/easylistgermany.txt",
        https_server_.port());
    list_hu_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/hufilter.txt",
        https_server_.port());
    list_pt_manual_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/easylistportuguese.txt",
        https_server_.port());
    list_easylist_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/easylist.txt",
        https_server_.port());
    list_exceptionrules_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/exceptionrules.txt",
        https_server_.port());
    list_anticv_ = base::StringPrintf(
        "https://easylist-downloads.adblockplus.org:%d/abp-filters-anti-cv.txt",
        https_server_.port());
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(request.relative_url, "/easylist.txt") ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt") ||
         base::StartsWith(request.relative_url, "/easylistportuguese.txt") ||
         base::StartsWith(request.relative_url, "/easylistpolish.txt") ||
         base::StartsWith(request.relative_url, "/easylistgermany.txt") ||
         base::StartsWith(request.relative_url, "/hufilter.txt"))) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content("[Adblock Plus 2.0]");
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    } else if (base::StartsWith(request.relative_url,
                                "/recommendations.json") &&
               !recommendations_json_.empty()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(recommendations_json_);
      http_response->set_content_type("text/plain");
      recommendations_json_ = "";
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

  void OnFilterListsChanged(adblock::FilteringConfiguration* config) override {
    // For the sake of current implementation we don't need to check what
    // has changed here as we use this check for one case and we do the
    // verification in test body
    if (filter_list_removed_closure_) {
      filter_list_removed_closure_.Run();
    }
  }

  void SetUpOnMainThread() override {
    AdblockFilterListDownloadTestBase::SetUpOnMainThread();
    configuration_ =
        SubscriptionServiceFactory::GetForBrowserContext(browser_context())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    configuration_->AddObserver(this);

    // Verify starting condition - no language based list installed
    ASSERT_EQ(3u, configuration_->GetFilterLists().size());
    EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_pl_}));
    EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_de_}));
    EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_hu_}));
  }

 protected:
  std::string list_easylist_;
  std::string list_exceptionrules_;
  std::string list_anticv_;
  std::string list_pl_;
  std::string list_de_;
  std::string list_hu_;
  std::string list_pt_manual_;
  static constexpr char recommendations_json_pattern_[] = "[{\"url\": \"%s\"}]";
  static constexpr char recommendations_json_pattern_2_[] =
      "[{\"url\": \"%s\"}, {\"url\": \"%s\"}]";
  std::string recommendations_json_;
  raw_ptr<FilteringConfiguration, DanglingUntriaged> configuration_;
  base::RepeatingClosure filter_list_removed_closure_;
};

IN_PROC_BROWSER_TEST_F(AdblockGeolocatedFilterListsTest,
                       CheckRecommendationsChange) {
  std::vector<GURL> subscriptions;

  // Add non auto installed filter list
  {
    subscriptions = {GURL{list_pt_manual_}};
    auto waiter = GetSubscriptionInstalledWaiter();
    configuration_->AddFilterList(GURL{list_pt_manual_});
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  ASSERT_EQ(4u, configuration_->GetFilterLists().size());
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_easylist_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_exceptionrules_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_anticv_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pt_manual_}));

  // Let's push two lists in recommendations.json
  {
    subscriptions = {GURL{list_de_}, GURL{list_pl_}};
    auto waiter = GetSubscriptionInstalledWaiter();
    recommendations_json_ = base::StringPrintf(recommendations_json_pattern_2_,
                                               subscriptions[0].spec().c_str(),
                                               subscriptions[1].spec().c_str());
    base::subtle::ScopedTimeClockOverrides time_override(
        []() { return TimeNowIgnoringOverride() + base::Days(1); }, nullptr,
        nullptr);
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  // Now we should also have two lists from recommendations.json
  ASSERT_EQ(6u, configuration_->GetFilterLists().size());
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pl_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_de_}));
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_hu_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pt_manual_}));

  // Let's push new single list in recommendations.json and advance
  // clocks to make previous two auto installed lists expired
  {
    subscriptions = {GURL{list_hu_}};
    auto waiter = GetSubscriptionInstalledWaiter();
    recommendations_json_ =
        base::StringPrintf(recommendations_json_pattern_, list_hu_.c_str());
    base::subtle::ScopedTimeClockOverrides time_override(
        []() {
          return TimeNowIgnoringOverride() + base::Days(1) + base::Days(14);
        },
        nullptr, nullptr);
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  // Now we should have one new list added and two previous auto installed
  // lists removed
  ASSERT_EQ(5u, configuration_->GetFilterLists().size());
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_pl_}));
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_de_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_hu_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pt_manual_}));

  // Let's advance the clocks to make remaining auto installed list expired
  // and removed
  {
    recommendations_json_ = "[]";
    base::subtle::ScopedTimeClockOverrides time_override(
        []() {
          return TimeNowIgnoringOverride() + base::Days(1) + base::Days(14) +
                 base::Days(14);
        },
        nullptr, nullptr);
    base::RunLoop run_loop;
    filter_list_removed_closure_ = run_loop.QuitClosure();
    std::move(run_loop).Run();
    ASSERT_EQ(4u, configuration_->GetFilterLists().size());
    EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_hu_}));
    EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pt_manual_}));
  }
}

IN_PROC_BROWSER_TEST_F(AdblockGeolocatedFilterListsTest,
                       VerifyRecommendedNotRemovedAheadOfTime) {
  std::vector<GURL> subscriptions;

  // Let's push a list_hu_ only in recommendations.json
  {
    subscriptions = {GURL{list_hu_}};
    auto waiter = GetSubscriptionInstalledWaiter();
    recommendations_json_ =
        base::StringPrintf(recommendations_json_pattern_, list_hu_.c_str());
    base::subtle::ScopedTimeClockOverrides time_override(
        []() { return TimeNowIgnoringOverride() + base::Days(1); }, nullptr,
        nullptr);
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  // Now we should also have a lists from recommendations.json
  ASSERT_EQ(4u, configuration_->GetFilterLists().size());
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_easylist_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_exceptionrules_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_anticv_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_hu_}));

  // Change recommendation to list_pl_ only
  {
    subscriptions = {GURL{list_pl_}};
    auto waiter = GetSubscriptionInstalledWaiter();
    recommendations_json_ =
        base::StringPrintf(recommendations_json_pattern_, list_pl_.c_str());
    base::subtle::ScopedTimeClockOverrides time_override(
        []() {
          return TimeNowIgnoringOverride() + base::Days(1) + base::Days(12);
        },
        nullptr, nullptr);
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  // Recommendation changed but the old list does not get removed before 14
  // daysRecommendation changed but the old list does not get removed before 14
  // days
  ASSERT_EQ(5u, configuration_->GetFilterLists().size());
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_hu_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pl_}));

  // The originally recommended list is removed after 14 days
  base::subtle::ScopedTimeClockOverrides time_override(
      []() {
        return TimeNowIgnoringOverride() + base::Days(1) + base::Days(14);
      },
      nullptr, nullptr);

  recommendations_json_ = "[]";
  base::RunLoop run_loop;
  filter_list_removed_closure_ = run_loop.QuitClosure();
  std::move(run_loop).Run();

  ASSERT_EQ(4u, configuration_->GetFilterLists().size());
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_hu_}));
  EXPECT_TRUE(configuration_->IsFilterListPresent(GURL{list_pl_}));
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_de_}));
  EXPECT_FALSE(configuration_->IsFilterListPresent(GURL{list_pt_manual_}));
}

}  // namespace adblock
