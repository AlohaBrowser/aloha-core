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

#include <memory>
#include <string_view>
#include <unordered_map>

#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/acceptable_ads_stats.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/adblock_telemetry_service_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "gtest/gtest.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {
namespace {

struct ExpectedPageViewCount {
  int aa_count;
  int total_count;
};

}  // namespace

class AdblockAcceptableAdsStatsBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockAcceptableAdsStatsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    // TODO(mpawlowski): Eyeometry endpoint address will need to be added to
    // dns_names too in DPD-2794.
    cert_config.dns_names = {"easylist-downloads.adblockplus.org",
                             "example.com"};
    https_server_.SetSSLConfig(cert_config);

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdblockAcceptableAdsStatsBrowserTest::RequestHandler,
        base::Unretained(this)));
    EXPECT_TRUE(https_server_.Start());

    SetFilterListServerPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Remove all pre-installed filter lists to avoid a race. We will re-add
    // them in the test, ensuring that we set up the mock responses first.
    RemoveAllDefaultFilterLists();
    MakeAcceptableAdsStats();
    DCHECK(acceptable_ads_stats_);
  }

  void TearDownOnMainThread() override {
    AdblockBrowserTestBase::TearDownOnMainThread();
    // The AcceptableAdsStats instance is owned by AdblockTelemetryService,
    // which is a BrowserContextKeyedService. It will be deleted when the
    // browser context is destroyed.
    acceptable_ads_stats_ = nullptr;
  }

  // This will happen in AdblockTelemetryServiceFactory in DPD-2794 and we'll no
  // longer be able to retain access to the AcceptableAdsStats pointer. The test
  // will need modifications by then.
  void MakeAcceptableAdsStats() {
    auto acceptable_ads_stats = std::make_unique<AcceptableAdsStats>(
        ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser_context()),
        GetPrefs());
    acceptable_ads_stats_ = acceptable_ads_stats.get();
    AdblockTelemetryServiceFactory::GetForBrowserContext(browser_context())
        ->AddTopicProvider(std::move(acceptable_ads_stats));
  }

  void RemoveAllDefaultFilterLists() {
    for (const auto& subscription :
         GetAdblockFilteringConfiguration()->GetFilterLists()) {
      GetAdblockFilteringConfiguration()->RemoveFilterList(subscription);
    }
  }

  void AddEasylistFilters(std::string_view filters) {
    // Prepare the response (returned by RequestHandler) to contain the filters
    // needed for the test.
    mock_easylist_filters_ = "[Adblock Plus 2.0]\n" + std::string(filters);
    // Add the filter list to the configuration, this will trigger a download.
    GetAdblockFilteringConfiguration()->AddFilterList(DefaultSubscriptionUrl());
    auto waiter = GetSubscriptionInstalledWaiter();
    waiter.WaitUntilSubscriptionsInstalled({DefaultSubscriptionUrl()});
  }

  void AddExceptionrulesFilters(std::string_view filters) {
    mock_exceptionrules_filters_ =
        "[Adblock Plus 2.0]\n" + std::string(filters);
    GetAdblockFilteringConfiguration()->AddFilterList(AcceptableAdsUrl());
    auto waiter = GetSubscriptionInstalledWaiter();
    waiter.WaitUntilSubscriptionsInstalled({AcceptableAdsUrl()});
  }

  void RegisterHtmlContent(std::string_view path, std::string_view content) {
    mock_websites_.push_back({path, content});
  }

  std::unique_ptr<net::test_server::BasicHttpResponse> RespondWithContent(
      std::string_view content,
      std::string_view content_type) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content(content);
    http_response->set_content_type(content_type);
    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (base::StartsWith(request.relative_url, AcceptableAdsUrl().path())) {
      return RespondWithContent(mock_exceptionrules_filters_, "text/plain");
    } else if (base::StartsWith(request.relative_url,
                                DefaultSubscriptionUrl().path())) {
      return RespondWithContent(mock_easylist_filters_, "text/plain");
    }
    const auto website = base::ranges::find_if(
        mock_websites_, [&request](const MockWebsiteContent& website) {
          return base::StartsWith(request.relative_url, website.url_path);
        });
    if (website != mock_websites_.end()) {
      return RespondWithContent(website->html_content, "text/html");
    }
    LOG(INFO) << "Unhandled request: " << request.relative_url;
    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  void VerifyPageViewCount(ExpectedPageViewCount expected) {
    // Parses the payload returned by AcceptableAdsStats::GetPayload() to infer
    // the page views count.
    base::MockCallback<AcceptableAdsStats::PayloadCallback> payload_callback;
    EXPECT_CALL(payload_callback,
                Run(base::NumberToString(expected.aa_count) + " " +
                    base::NumberToString(expected.total_count)));
    acceptable_ads_stats_->GetPayload(payload_callback.Get());
  }

  void NavigateToPage(GURL url) {
    // Replace the port to match the EmbeddedTestServer.
    GURL::Replacements replacements;
    const std::string port_str = base::NumberToString(https_server_.port());
    replacements.SetPortStr(port_str);
    const GURL new_url = url.ReplaceComponents(replacements);
    // Navigate. Whatever the domain was, it will be redirected to localhost
    // due to the host resolver rule. Filter matching will still see the
    // original URL.
    ASSERT_TRUE(content::NavigateToURL(shell(), new_url));
  }

 protected:
  FilteringConfiguration* GetAdblockFilteringConfiguration() {
    return SubscriptionServiceFactory::GetForBrowserContext(browser_context())
        ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  }
  struct MockWebsiteContent {
    std::string_view url_path;  // All URLs are relative to localhost, only the
                                // path matters. Eg. "/test_page.html"
    std::string_view html_content;
  };
  net::EmbeddedTestServer https_server_;
  std::string mock_easylist_filters_;
  std::string mock_exceptionrules_filters_;
  std::vector<MockWebsiteContent> mock_websites_;

  // TODO(mpawlowski): This will be replaced with a mechanism to reliably
  // trigger eyeometry requests in DPD-2794. For now, we manually add
  // AcceptableAdsStats to AdblockTelemetryService and we're able to retain the
  // pointer. Later it will be added in AdblockTelemetryServiceFactory, and
  // we'll no longer have this access, we'll need to monitor eyeometry requests.
  raw_ptr<AcceptableAdsStats> acceptable_ads_stats_;
};

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       SiteWithNoBlockedRequestDoesNotCountAsAA) {
  // There are some filters defined...
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@resource.png");
  // But none of them hit on this page:
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="image.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This is not an Acceptable Ads page view, but it does count towards total
  // page views count.
  VerifyPageViewCount({.aa_count = 0, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       SiteWithNoAllowlistingDoesNotCountAsAA) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("");
  // The image request is blocked, it's not allowlisted by AA.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));

  VerifyPageViewCount({.aa_count = 0, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       ResourceAllowedByExceptionrulesCountsAsAA) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  // The image request is blocked by easylist but allowlisted by AA.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       AllowingFilterFromEasylistDoesNotCountAsAA) {
  AddEasylistFilters(R"(
    blocked_resource.png
    @@.png
  )");
  AddExceptionrulesFilters("");
  // The image request is blocked by easylist, but also allowlisted by easylist.
  // No AA filter is hit.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This is not an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 0, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       MultipleAllowedResourcesCountAsSingleAAPV) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  // The image request is blocked by easylist but allowlisted by AA.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as a single Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       NavigatingToTheSiteAgainCountsAsNewAAPV) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  // The image request is blocked by easylist but allowlisted by AA.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
  // Navigating to the same page again counts as a new Acceptable Ads page view.
  NavigateToPage(GURL("https://example.com/test_page.html"));
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(
    AdblockAcceptableAdsStatsBrowserTest,
    ResourceAllowedWithinIframeCountsTowardsParentFramesAAPV) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  // The main frame loads an iframe.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <iframe src="iframe.html"></iframe>
      </body>
    </html>
  )");
  // The iframe loads an image that is blocked by easylist but allowlisted by
  // AA.
  RegisterHtmlContent("/iframe.html", R"(
    <html>
      <head>
        <title>Test iframe</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(
    AdblockAcceptableAdsStatsBrowserTest,
    MultipleResourcesAllowedAcrossMultipleFramesCountTowardSingleAAPV) {
  AddEasylistFilters(R"(
    blocked_resource.png
    blocked_ad.png
  )");
  AddExceptionrulesFilters("@@blocked*.png");
  // The main frame loads 2 iframes and some blocked ad.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <iframe src="iframe.html"></iframe>
        <iframe src="iframe.html"></iframe>
        <img src="blocked_ad.png">
      </body>
    </html>
  )");
  // The iframe loads an image that is blocked by easylist but allowlisted by
  // AA.
  RegisterHtmlContent("/iframe.html", R"(
    <html>
      <head>
        <title>Test iframe</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // There are 3 resources allowed by AA, but they all count as a single AAPV.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       WebsiteAllowlistedWithDocumentFilterCountsTowardAAPV) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@example.com$document");
  // The main frame is allowlisted by a document filter (full page allowlist).
  // It loads an iframe with blocked content.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <iframe src="iframe.html"></iframe>
      </body>
    </html>
  )");
  RegisterHtmlContent("/iframe.html", R"(
    <html>
      <head>
        <title>Test iframe</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       IframeAllowlistedWithSubdocumentFilterCountsTowardAAPV) {
  AddEasylistFilters("iframe.html");
  AddExceptionrulesFilters("@@iframe.html$subdocument");
  // The main frame loads an iframe that is allowlisted by a subdocument filter.
  // The iframe contains blocked content.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <iframe src="iframe.html"></iframe>
      </body>
    </html>
  )");
  RegisterHtmlContent("/iframe.html", R"(
    <html>
      <head>
        <title>Test iframe</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  // This counts as an Acceptable Ads page view.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       CountResetAfterSuccessfulEyeometryRequest) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  // Accumulate 2 AA page views:
  NavigateToPage(GURL("https://example.com/test_page.html"));
  NavigateToPage(GURL("https://example.com/test_page.html"));
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
  // TODO(mpawlowski): This will be changed to a real response in DPD-2794.
  acceptable_ads_stats_->ParseResponse(std::make_unique<std::string>("OK"));

  // The count is reset after a successful eyeometry request.
  VerifyPageViewCount({.aa_count = 0, .total_count = 0});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       CountNotResetAfterFailedEyeometryRequest) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  // Accumulate 2 AA page views:
  NavigateToPage(GURL("https://example.com/test_page.html"));
  NavigateToPage(GURL("https://example.com/test_page.html"));
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});

  // TODO(mpawlowski): This will be changed to a real failed response in
  // DPD-2794.
  acceptable_ads_stats_->ParseResponse(nullptr);

  // The count is not reset after a failed eyeometry request. We will try to
  // report this count again later.
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       PRE_CountIsRetainedBetweenRestartedBrowserSessions) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  // Accumulate 2 AA page views:
  NavigateToPage(GURL("https://example.com/test_page.html"));
  NavigateToPage(GURL("https://example.com/test_page.html"));
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockAcceptableAdsStatsBrowserTest,
                       CountIsRetainedBetweenRestartedBrowserSessions) {
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

}  // namespace adblock
