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

#include <string>

#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::HasSubstr;
using testing::Mock;
using testing::Return;
using testing::StartsWith;

namespace adblock {

class AdblockDebugUrlTest : public AdblockBrowserTestBase {
 public:
  AdblockDebugUrlTest() {}
  ~AdblockDebugUrlTest() override = default;
  AdblockDebugUrlTest(const AdblockDebugUrlTest&) = delete;
  AdblockDebugUrlTest& operator=(const AdblockDebugUrlTest&) = delete;

 protected:
  std::string ExecuteScriptAndExtractString(const std::string& js_code) {
    ;
    return content::EvalJs(web_contents(), js_code).ExtractString();
  }

  bool IsAdblockEnabled() {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser_context())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    DCHECK(adblock_configuration) << "Test expects \"adblock\" configuration";
    return adblock_configuration->IsEnabled();
  }

  bool IsAAEnabled() {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser_context())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    DCHECK(adblock_configuration) << "Test expects \"adblock\" configuration";
    return base::ranges::any_of(
        adblock_configuration->GetFilterLists(),
        [&](const auto& url) { return url == AcceptableAdsUrl(); });
  }

  std::string GetUrlForAdblockConfiguration() {
    return std::string("http://") + kAdblockFilteringConfigurationName + "." +
           AdblockURLLoaderFactoryForTest::kEyeoDebugDataHostName;
  }

  std::string GetUrlForListingConfigurations() {
    return std::string("http://") +
           AdblockURLLoaderFactoryForTest::kEyeoDebugDataHostName +
           "/configurations";
  }

  const std::string kReadPageBodyScript =
      "document.getElementsByTagName('body')[0].firstChild.innerHTML";
};

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestInvalidUrls) {
  GURL no_command1(GetUrlForAdblockConfiguration());
  ASSERT_TRUE(content::NavigateToURL(shell(), no_command1));
  ASSERT_TRUE(base::StartsWith(
      ExecuteScriptAndExtractString(kReadPageBodyScript), "INVALID_COMMAND"));

  GURL no_command2(GetUrlForAdblockConfiguration() + "/");
  ASSERT_TRUE(content::NavigateToURL(shell(), no_command2));
  ASSERT_TRUE(base::StartsWith(
      ExecuteScriptAndExtractString(kReadPageBodyScript), "INVALID_COMMAND"));

  GURL invalid_command_url(GetUrlForAdblockConfiguration() +
                           "/some_invalid_command");
  ASSERT_TRUE(content::NavigateToURL(shell(), invalid_command_url));
  ASSERT_TRUE(base::StartsWith(
      ExecuteScriptAndExtractString(kReadPageBodyScript), "INVALID_COMMAND"));

  GURL invalid_topic(GetUrlForAdblockConfiguration() +
                     "/filter/add/%2Fadsponsor.");
  ASSERT_TRUE(content::NavigateToURL(shell(), invalid_topic));
  ASSERT_TRUE(base::StartsWith(
      ExecuteScriptAndExtractString(kReadPageBodyScript), "INVALID_COMMAND"));

  GURL invalid_command(GetUrlForAdblockConfiguration() +
                       "/filters/ad/%2Fadsponsor.");
  ASSERT_TRUE(content::NavigateToURL(shell(), invalid_command));
  ASSERT_TRUE(base::StartsWith(
      ExecuteScriptAndExtractString(kReadPageBodyScript), "INVALID_COMMAND"));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestFilterCommands) {
  GURL clear_filters_url(GetUrlForAdblockConfiguration() + "/filters/clear");
  ASSERT_TRUE(content::NavigateToURL(shell(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_filters_url(GetUrlForAdblockConfiguration() + "/filters/list");
  ASSERT_TRUE(content::NavigateToURL(shell(), list_filters_url));
  std::string expected_no_filters = "OK";
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_filters_url(GetUrlForAdblockConfiguration() +
                       "/filters/add/%2FadsPlugin%2F%2A%0A%2Fadsponsor.");
  ASSERT_TRUE(content::NavigateToURL(shell(), add_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_filters_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("adsPlugin/*"));
  ASSERT_THAT(response, HasSubstr("adsponsor."));

  GURL remove_filter_url(GetUrlForAdblockConfiguration() +
                         "/filters/remove/%2Fadsponsor.");
  ASSERT_TRUE(content::NavigateToURL(shell(), remove_filter_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_filters_url));
  std::string expected_one_filter = "OK\n\n/adsPlugin/*\n";
  ASSERT_EQ(expected_one_filter,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_filters_url));
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestDomainCommands) {
  GURL clear_domains_url(GetUrlForAdblockConfiguration() + "/domains/clear");
  ASSERT_TRUE(content::NavigateToURL(shell(), clear_domains_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_domains_url(GetUrlForAdblockConfiguration() + "/domains/list");
  ASSERT_TRUE(content::NavigateToURL(shell(), list_domains_url));
  std::string expected_no_domains = "OK";
  ASSERT_EQ(expected_no_domains,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_domain_url(GetUrlForAdblockConfiguration() +
                      "/domains/add/example.com%0Adomain.org");
  ASSERT_TRUE(content::NavigateToURL(shell(), add_domain_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_domains_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("example.com"));
  ASSERT_THAT(response, HasSubstr("domain.org"));

  GURL remove_domain_url(GetUrlForAdblockConfiguration() +
                         "/domains/remove/example.com");
  ASSERT_TRUE(content::NavigateToURL(shell(), remove_domain_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_domains_url));
  std::string expected_one_domain = "OK\n\ndomain.org\n";
  ASSERT_EQ(expected_one_domain,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), clear_domains_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_domains_url));
  ASSERT_EQ(expected_no_domains,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestSubscriptionCommands) {
  GURL clear_subscriptions_url(GetUrlForAdblockConfiguration() +
                               "/subscriptions/clear");
  ASSERT_TRUE(content::NavigateToURL(shell(), clear_subscriptions_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_subscriptions_url(GetUrlForAdblockConfiguration() +
                              "/subscriptions/list");
  ASSERT_TRUE(content::NavigateToURL(shell(), list_subscriptions_url));
  std::string expected_no_subscriptions = "OK";
  ASSERT_EQ(expected_no_subscriptions,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_subscription_url(GetUrlForAdblockConfiguration() +
                            "/subscriptions/add/"
                            "https%3A%2F%2Fexample.com%2Flist1.txt%0Ahttps%3A%"
                            "2F%2Fwww.domain.org%2Flist2.txt");
  ASSERT_TRUE(content::NavigateToURL(shell(), add_subscription_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_subscriptions_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("https://example.com/list1.txt"));
  ASSERT_THAT(response, HasSubstr("https://www.domain.org/list2.txt"));

  GURL remove_subscription_url(
      GetUrlForAdblockConfiguration() +
      "/subscriptions/remove/https%3A%2F%2Fwww.domain.org%2Flist2.txt");
  ASSERT_TRUE(content::NavigateToURL(shell(), remove_subscription_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_subscriptions_url));
  std::string expected_one_subscription =
      "OK\n\nhttps://example.com/list1.txt\n";
  ASSERT_EQ(expected_one_subscription,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), clear_subscriptions_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), list_subscriptions_url));
  ASSERT_EQ(expected_no_subscriptions,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestEnableConfigurationCommands) {
  GURL enable_adblock__url(GetUrlForAdblockConfiguration() +
                           "/configuration/enable");
  GURL disable_adblock_url(GetUrlForAdblockConfiguration() +
                           "/configuration/disable");
  GURL adblock_state_url(GetUrlForAdblockConfiguration() +
                         "/configuration/state");

  ASSERT_TRUE(IsAdblockEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), adblock_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), disable_adblock_url));
  ASSERT_FALSE(IsAdblockEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), adblock_state_url));
  ASSERT_EQ("OK\n\ndisabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), enable_adblock__url));
  ASSERT_TRUE(IsAdblockEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), adblock_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestEnableAACommands) {
  GURL enable_aa_url(GetUrlForAdblockConfiguration() + "/aa/enable");
  GURL disable_aa_url(GetUrlForAdblockConfiguration() + "/aa/disable");
  GURL aa_state_url(GetUrlForAdblockConfiguration() + "/aa/state");

  ASSERT_TRUE(IsAAEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), aa_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), disable_aa_url));
  ASSERT_FALSE(IsAAEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), aa_state_url));
  ASSERT_EQ("OK\n\ndisabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(content::NavigateToURL(shell(), enable_aa_url));
  ASSERT_TRUE(IsAAEnabled());
  ASSERT_TRUE(content::NavigateToURL(shell(), aa_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestListConfigurationsCommands) {
  GURL list_configurations_url(GetUrlForListingConfigurations());
  ASSERT_TRUE(content::NavigateToURL(shell(), list_configurations_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("adblock"));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestUrlsInterception) {
  std::vector<GURL> invalid_urls = {
      GURL{"https://adblocktest.data"}, GURL{"https://adblock.testdata"},
      GURL{"https://adblock.test.data.eyeo"}, GURL{"https://test.data.eyeo"}};
  std::vector<GURL> valid_urls = {GURL{"https://adblock.test.data"},
                                  GURL{"https://ad.block.test.data"}};
  for (const auto& url : invalid_urls) {
    ASSERT_FALSE(content::NavigateToURL(shell(), url));
  }
  for (const auto& url : valid_urls) {
    ASSERT_TRUE(content::NavigateToURL(shell(), url));
  }
}

}  // namespace adblock
