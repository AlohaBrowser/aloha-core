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

#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockAcceptableAdsTest
    : public AdblockBrowserTestBase,
      public testing::WithParamInterface<std::tuple<bool, bool, bool>> {
 public:
  AdblockAcceptableAdsTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
    setenv("LANGUAGE", "en_US", 1);
#endif
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdblockAcceptableAdsTest::RequestHandler, base::Unretained(this)));
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org", kTestDomain};
    https_server_.SetSSLConfig(cert_config);
    EXPECT_TRUE(https_server_.Start());
    SetFilterListServerPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("easylist-downloads.adblockplus.org", "127.0.0.1");
    host_resolver()->AddRule(kTestDomain, "127.0.0.1");
    if (DomainAllowlisted()) {
      auto* adblock_configuration =
          SubscriptionServiceFactory::GetForBrowserContext(browser_context())
              ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
      DCHECK(adblock_configuration);
      adblock_configuration->AddAllowedDomain(kTestDomain);
    }
    InitResourceClassificationObserver();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (!AcceptableAdsEnabled()) {
      command_line->AppendSwitch(switches::kDisableAcceptableAds);
    }
    if (IncognitoMode()) {
      command_line->AppendSwitch("incognito");
    }
  }

  void WaitUntilSubscriptionsInstalled() {
    std::vector<GURL> subscriptions = {DefaultSubscriptionUrl()};
    if (AcceptableAdsEnabled()) {
      subscriptions.emplace_back(AcceptableAdsUrl());
    }
    auto waiter = GetSubscriptionInstalledWaiter();
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() == AcceptableAdsUrl().path()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(
          "[Adblock Plus 2.0]\n\n"
          "@@iframe_image.png");
      return std::move(http_response);
    } else if (request.GetURL().path() == DefaultSubscriptionUrl().path()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(
          "[Adblock Plus 2.0]\n\n"
          "iframe_image.png");
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    } else if (base::StartsWith(request.relative_url, "/test_page.html")) {
      static constexpr char kMainFrame[] =
          R"(
        <!DOCTYPE html>
        <html>
          <body>
            <iframe src="sitekey_iframe.html?query"></iframe>
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kMainFrame);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    } else if (base::StartsWith(request.relative_url, "/sitekey_iframe.html")) {
      static constexpr char kIframe[] =
          R"(
        <!DOCTYPE html>
        <html>
          <body>
            <img src="/iframe_image.png" />
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kIframe);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  GURL GetPageUrl(const std::string& path = "/test_page.html") {
    return https_server_.GetURL(kTestDomain, path);
  }

  void NavigateToPage() {
    ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  }

  void VerifyExpectedNotifications() {
    if (AcceptableAdsEnabled() || DomainAllowlisted()) {
      ASSERT_EQ(observer_.allowed_ads_notifications.size(), 1u);
      EXPECT_TRUE(observer_.allowed_ads_notifications.front() ==
                  GetPageUrl("/iframe_image.png"))
          << "Request not allowed!";
      if (DomainAllowlisted()) {
        ASSERT_EQ(observer_.allowed_pages_notifications.size(), 1u);
        EXPECT_TRUE(observer_.allowed_pages_notifications.front() ==
                    GetPageUrl())
            << "Page not allowed!";
      }
    } else {
      ASSERT_EQ(observer_.blocked_ads_notifications.size(), 1u);
      EXPECT_TRUE(observer_.blocked_ads_notifications.front() ==
                  GetPageUrl("/iframe_image.png"))
          << "Request not blocked!";
      EXPECT_TRUE(observer_.allowed_ads_notifications.empty());
      EXPECT_TRUE(observer_.allowed_pages_notifications.empty());
    }
  }

  bool AcceptableAdsEnabled() { return std::get<0>(GetParam()); }

  bool DomainAllowlisted() { return std::get<1>(GetParam()); }

  bool IncognitoMode() { return std::get<2>(GetParam()); }

 private:
  net::EmbeddedTestServer https_server_;
  static constexpr char kTestDomain[] = "test.org";
};

IN_PROC_BROWSER_TEST_P(AdblockAcceptableAdsTest, VerifyAcceptableAds) {
  LOG(INFO) << "AA on: " << AcceptableAdsEnabled();
  LOG(INFO) << "Domain allowed: " << DomainAllowlisted();
  LOG(INFO) << "Incognito: " << IncognitoMode();
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration);
  auto subscriptions = adblock_configuration->GetFilterLists();
  // This remove/add dance is required to avoid race when we are not sure
  // if subscriptions were already installed or not. It's difficult to set
  // SubscriptionObserver for built-in subscriptions in right time during
  // test setup so we do it here and by Remove() then Add() we trigger
  // filter lists installations which we then observe and block until done.
  for (const auto& subscription : subscriptions) {
    adblock_configuration->RemoveFilterList(subscription);
    adblock_configuration->AddFilterList(subscription);
  }
  WaitUntilSubscriptionsInstalled();
  NavigateToPage();
  VerifyExpectedNotifications();
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockAcceptableAdsTest,
    testing::Combine(/* AA on/off */ testing::Bool(),
                     /* Allowlist domain */ testing::Bool(),
                     /* Incognito on/off */ testing::Bool()));

}  // namespace adblock
