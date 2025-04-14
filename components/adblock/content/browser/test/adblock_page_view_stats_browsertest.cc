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

#include "base/json/json_reader.h"
#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/adblock_telemetry_service_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/activeping_telemetry_topic_provider.h"
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
  int aa_bt_count;
  int allowing_count;
  int blocking_count;
  int total_count;
};

enum ServerRespondsWith {
  Ok,
  Error,
};

}  // namespace

class AdblockPageViewStatsBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockPageViewStatsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org",
                             GetTelemetryDomain(), "ad-delivery.net",
                             "btloader.com", "example.com"};
    https_server_.SetSSLConfig(cert_config);

    https_server_.RegisterRequestHandler(
        base::BindRepeating(&AdblockPageViewStatsBrowserTest::RequestHandler,
                            base::Unretained(this)));
    EXPECT_TRUE(https_server_.Start());

    SetFilterListServerPortForTesting(https_server_.port());

    ActivepingTelemetryTopicProvider::SetHttpsPortForTesting(
        https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Remove all pre-installed filter lists to avoid a race. We will re-add
    // them in the test, ensuring that we set up the mock responses first.
    RemoveAllDefaultFilterLists();
  }

  void TearDownOnMainThread() override {
    AdblockBrowserTestBase::TearDownOnMainThread();
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

  void AddCustomFilters(std::vector<std::string> filters) {
    for (const auto& filter : filters) {
      GetAdblockFilteringConfiguration()->AddCustomFilter(filter);
    }
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

  std::unique_ptr<net::test_server::BasicHttpResponse> RedirectToPage(
      const std::string& redirect_to_target) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader("Location", redirect_to_target);
    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> CreateTelemetryResponse() {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    if (expected_response_ == ServerRespondsWith::Error) {
      http_response->set_code(net::HTTP_NOT_FOUND);
    } else {
      http_response->set_code(net::HTTP_OK);
      http_response->set_content("{\"token\": \"dummy\"}");
      http_response->set_content_type("text/plain");
    }
    return std::move(http_response);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (base::StartsWith(request.relative_url, AcceptableAdsUrl().path())) {
      return RespondWithContent(mock_exceptionrules_filters_, "text/plain");
    } else if (base::StartsWith(request.relative_url,
                                DefaultSubscriptionUrl().path())) {
      return RespondWithContent(mock_easylist_filters_, "text/plain");
    }
    if (base::StartsWith(request.relative_url, "/recovery?w=")) {
      return RespondWithContent("", "text/plain");
    }
    if (base::StartsWith(request.relative_url, "/px.gif?ch=")) {
      return RespondWithContent("", "image/gif");
    }
    if (base::StartsWith(request.relative_url, "/redirect_to")) {
      GURL redirect_target("https://example.com/" + request.GetURL().query());
      const GURL new_url = GetUrlmatchingServerPort(redirect_target);
      return RedirectToPage(new_url.spec());
    }

    if (base::StartsWith(request.relative_url,
                         "/topic/eyeochromium_activeping/version/2")) {
      EXPECT_TRUE(request.has_content);
      telemetry_response_ = base::JSONReader::Read(request.content);
      NotifyTestFinished();
      return CreateTelemetryResponse();
    }
    const auto website = std::ranges::find_if(
        mock_websites_, [&request](const MockWebsiteContent& website) {
          return base::StartsWith(request.relative_url, website.url_path);
        });
    if (website != mock_websites_.end()) {
      return RespondWithContent(website->html_content, "text/html");
    }
    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  void VerifyPageViewCount(ExpectedPageViewCount expected) {
    auto* dict_payload = GetPayload();
    auto* value = dict_payload->Find("aa_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.aa_count, value->GetInt());

    value = dict_payload->Find("aa_bt_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.aa_bt_count, value->GetInt());

    value = dict_payload->Find("allowed_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.allowing_count, value->GetInt());

    value = dict_payload->Find("blocked_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.blocking_count, value->GetInt());

    value = dict_payload->Find("pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.total_count, value->GetInt());
  }

  void NavigateToPage(GURL start_url,
                      GURL redirection_url = GURL(/*empty invalid url*/)) {
    const GURL new_start_url = GetUrlmatchingServerPort(start_url);
    // Navigate. Whatever the domain was, it will be redirected to localhost
    // due to the host resolver rule. Filter matching will still see the
    // original URL.
    if (redirection_url.is_valid()) {
      const GURL new_final_url = GetUrlmatchingServerPort(redirection_url);
      ASSERT_TRUE(
          content::NavigateToURL(shell(), new_start_url, new_final_url));
    } else {
      ASSERT_TRUE(content::NavigateToURL(shell(), new_start_url));
    }
  }

  // Calling TriggerTelemetry(ServerRespondsWith::Error) prevents page view
  // stats from being reset after triggering telemetry request.
  void TriggerTelemetry(
      ServerRespondsWith expected_response = ServerRespondsWith::Ok) {
    expected_response_ = expected_response;
    telemetry_response_ = absl::nullopt;
    finish_condition_met_ = false;
    AdblockTelemetryServiceFactory::GetForBrowserContext(browser_context())
        ->TriggerConversationsWithoutDueTimeCheckForTesting();
    RunUntilTestFinished();
  }

 protected:
  FilteringConfiguration* GetAdblockFilteringConfiguration() {
    return SubscriptionServiceFactory::GetForBrowserContext(browser_context())
        ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  }

  GURL GetUrlmatchingServerPort(const GURL& initial_url) {
    // Replace the port to match the EmbeddedTestServer.
    GURL::Replacements replacements;
    const std::string port_str = base::NumberToString(https_server_.port());
    replacements.SetPortStr(port_str);
    return initial_url.ReplaceComponents(replacements);
  }

  base::Value::Dict* GetPayload() {
    EXPECT_TRUE(telemetry_response_ && telemetry_response_->is_dict());
    base::Value::Dict* parsed_dict = telemetry_response_->GetIfDict();
    EXPECT_TRUE(parsed_dict);
    base::Value::Dict* payload = parsed_dict->FindDict("payload");
    EXPECT_TRUE(payload);
    return payload;
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
  absl::optional<base::Value> telemetry_response_ = absl::nullopt;
  ServerRespondsWith expected_response_ = ServerRespondsWith::Ok;
};

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       SiteWithNoBlockedRequestDoesNotCount) {
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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       SiteWithNoAllowlistingCountAsBlocked) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("");

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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ResourceAllowedByExceptionrulesCountsInTwoMetrics) {
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
  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ResourceAllowedByEasylistCountsInAllowlistingMetric) {
  AddEasylistFilters(R"(
    blocked_resource.png
    @@.png
  )");
  AddExceptionrulesFilters("");

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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ResourceAllowedByBlocktrhoughExceptionrulesCounted) {
  // Expected script signaling that we need to increase aa_bt_count is requested
  // when Easylist is detected (filter "||ad-delivery.net^") and
  // then AA is detected (filter "@@||ad-delivery.net*/px.gif?ch=1").
  std::string easylist_detect_filter = "||ad-delivery.net^";
  auto easylist_detect_request = base::StringPrintf(
      R"(https://ad-delivery.net:%d/px.gif?ch=2)", https_server_.port());
  std::string aa_detect_filter = "@@||ad-delivery.net*/px.gif?ch=1";
  auto aa_detect_request = base::StringPrintf(
      R"(https://ad-delivery.net:%d/px.gif?ch=1&e=0.09078094279406423)",
      https_server_.port());

  AddEasylistFilters(easylist_detect_filter);
  AddExceptionrulesFilters(aa_detect_filter);

  auto expected_aa_bt_count_request = base::StringPrintf(
      R"(https://btloader.com:%d/recovery?w=5742015956385792&upapi=true)",
      https_server_.port());

  // Here we just make all requests expected by the flow at once not in order,
  // but in real case easylist_detect_request is triggered and when blocked,
  // then aa_detect_request is triggered and when allowed, then
  // expected_aa_bt_count_request is triggered. Also in real scenario both
  // detection requests are triggered from a script not from a page directly.
  auto page = base::StringPrintf(R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="%s">
        <img src="%s">
        <script src=%s></script>
      </body>
    </html>
  )",
                                 easylist_detect_request.c_str(),
                                 aa_detect_request.c_str(),
                                 expected_aa_bt_count_request.c_str());
  RegisterHtmlContent("/test_page.html", page);
  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  // aa_bt_count is subcategory of aa_count (which is subcategory of
  // allowing_count) hence we increase once aa_bt_count, aa_count,
  // allowing_count. But also during detection blocking filter is hit and
  // blocking_count is increased.
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 1,
                       .allowing_count = 1,
                       .blocking_count = 1,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       MultipleAllowedResourcesCountAsSingleHit) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("@@blocked_resource.png");
  // The image request is blocked by easylist but allowlisted by AA.
  RegisterHtmlContent("/test_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png?param=1">
        <img src="blocked_resource.png?param=2">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       NavigatingToTheSiteAgainCountsAsNewHit) {
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

  TriggerTelemetry(ServerRespondsWith::Error);
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});

  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ResourceAllowedWithinIframeCountsTowardsParentFrameHit) {
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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(
    AdblockPageViewStatsBrowserTest,
    MultipleResourcesAllowedAcrossMultipleFramesCountTowardSingleHit) {
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
        <iframe src="iframe.html?param=1"></iframe>
        <iframe src="iframe.html?param=2"></iframe>
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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       WebsiteAllowlistedWithDocumentFilterCountsInTwoMetrics) {
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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(
    AdblockPageViewStatsBrowserTest,
    IframeAllowlistedWithSubdocumentFilterCountsInTwoMetrics) {
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

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       IframeBlockedWithSubdocumentFilterCounted) {
  AddEasylistFilters("iframe.html");
  // The main frame loads an iframe that is blocked by a subdocument filter.
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
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       PRE_CountResetAfterSuccessfulEyeometryRequest) {
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
  NavigateToPage(GURL("https://example.com/test_page.html"));
  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       CountResetAfterSuccessfulEyeometryRequest) {
  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 0});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
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

  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry(ServerRespondsWith::Error);
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});

  TriggerTelemetry(ServerRespondsWith::Error);
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});

  NavigateToPage(GURL("https://example.com/test_page.html"));

  TriggerTelemetry(ServerRespondsWith::Error);
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ServerRedirectionNotIncrementsPageCount) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("");
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
  NavigateToPage(GURL("https://example.com/redirect_to?test_page.html"),
                 GURL("https://example.com/test_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       ClientRedirectionIncrementsPageCount) {
  AddEasylistFilters("blocked_resource.png");
  AddExceptionrulesFilters("");
  RegisterHtmlContent("/before_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
        <script type="text/javascript">
          window.location = "/after_page.html"
        </script>
      </head>
    </html>
  )");
  RegisterHtmlContent("/after_page.html", R"(
    <html>
      <head>
        <title>Test page</title>
      </head>
      <body>
        <img src="blocked_resource.png">
      </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/before_page.html"),
                 GURL("https://example.com/after_page.html"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 2});
}

IN_PROC_BROWSER_TEST_F(AdblockPageViewStatsBrowserTest,
                       AboutBlankPageIsNotCounted) {
  NavigateToPage(GURL("about:blank"));

  TriggerTelemetry();
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 0});
}

}  // namespace adblock
