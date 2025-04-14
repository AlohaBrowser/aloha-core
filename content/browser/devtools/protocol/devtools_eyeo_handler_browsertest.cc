// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

#include <string>
#include <vector>

#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_keyed_service_ptr_holder.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/browser/devtools/protocol/devtools_protocol_test_support.h"
#include "content/browser/devtools/protocol/eyeo_handler.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/app/shell_main_delegate.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {
class SubscriptionInstalledWaiter
    : public adblock::SubscriptionService::SubscriptionObserver {
 public:
  explicit SubscriptionInstalledWaiter(
      adblock::SubscriptionService* subscription_service);

  ~SubscriptionInstalledWaiter() override;

  void WaitUntilSubscriptionsInstalled(std::vector<GURL> subscriptions);

  void OnSubscriptionInstalled(const GURL& subscription_url) override;

 protected:
  raw_ptr<adblock::SubscriptionService> subscription_service_;
  base::RunLoop run_loop_;
  std::vector<GURL> awaited_subscriptions_;
};
}  // namespace

class DevToolsEyeoHandlerBrowserTest : public DevToolsProtocolTest {
 public:
  DevToolsEyeoHandlerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"example.com"};
    https_server_.SetSSLConfig(cert_config);

    https_server_.RegisterRequestHandler(
        base::BindRepeating(&DevToolsEyeoHandlerBrowserTest::RequestHandler,
                            base::Unretained(this)));
    EXPECT_TRUE(https_server_.Start());
  }

  const base::Value::Dict* TriggerCommand(
      std::string cmd,
      std::map<std::string_view, std::string_view> params_map) {
    base::Value::Dict params;
    for (auto const& param_entry : params_map) {
      params.Set(param_entry.first, param_entry.second);
    }
    return SendCommandSync(cmd, std::move(params));
  }

  void VerifyError(
      const base::Value::Dict* result,
      crdtp::DispatchCode expected_code = crdtp::DispatchCode::SERVER_ERROR) {
    EXPECT_FALSE(result);
    EXPECT_THAT(error()->FindInt("code"),
                testing::Optional(static_cast<int>(expected_code)));
  }

  std::vector<std::string> ExtractResponseData(const base::Value::Dict* result,
                                               const std::string& key) {
    auto* data = result->FindList(key);
    EXPECT_TRUE(data);
    std::vector<std::string> actual_values;
    if (data) {
      for (base::Value::List::const_iterator it = data->cbegin();
           it != data->cend(); ++it) {
        actual_values.push_back(it->GetString());
      }
    }
    return actual_values;
  }

  void WaitForEventNotification(const std::string& event_name,
                                const std::string& data_key) {
    base::Value::Dict notification = WaitForNotification(event_name, true);
    EXPECT_EQ(*notification.FindStringByDottedPath("configuration"), "adblock");
  }

  void WaitForFilterHitEvent(const std::string& event_name, const GURL& url) {
    base::Value::Dict notification = WaitForNotification(event_name, true);
    EXPECT_EQ(*notification.FindStringByDottedPath("subscription"),
              adblock::CustomFiltersUrl().spec());
    EXPECT_EQ(*notification.FindStringByDottedPath("url"),
              GetUrlmatchingServerPort(url).spec());
  }

  void AddCustomFilters(std::vector<std::string> filters) {
    for (const auto& filter : filters) {
      GetAdblockFilteringConfiguration()->AddCustomFilter(filter);
    }
  }

  void RegisterHtmlContent(std::string_view path, std::string_view content) {
    mock_websites_.push_back({path, content});
  }

  void NavigateToPage(GURL start_url) {
    const GURL new_start_url = GetUrlmatchingServerPort(start_url);
    ASSERT_TRUE(content::NavigateToURL(shell(), new_start_url));
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

  content::ContentMainDelegate* GetOptionalContentMainDelegateOverride()
      override {
    return new content::ShellMainDelegate(true);
  }

 protected:
  adblock::FilteringConfiguration* GetAdblockFilteringConfiguration() {
    return adblock::KeyedServicePtrHolder<adblock::SubscriptionService>::Get(
               shell()->web_contents()->GetBrowserContext(),
               adblock::kSubscriptionServiceUserDataKey)
        ->GetFilteringConfiguration(
            adblock::kAdblockFilteringConfigurationName);
  }

  GURL GetUrlmatchingServerPort(const GURL& initial_url) {
    // Replace the port to match the EmbeddedTestServer.
    GURL::Replacements replacements;
    const std::string port_str = base::NumberToString(https_server_.port());
    replacements.SetPortStr(port_str);
    return initial_url.ReplaceComponents(replacements);
  }

  struct MockWebsiteContent {
    std::string_view url_path;  // All URLs are relative to localhost, only the
                                // path matters. Eg. "/test_page.html"
    std::string_view html_content;
  };

  static constexpr std::string_view kCustomList =
      "https://example.org/list.txt";
  static constexpr std::string_view kCustomFilter = "example.org##.ad";
  static constexpr std::string_view kAllowedDomain = "example.org";
  net::EmbeddedTestServer https_server_;
  std::string mock_easylist_filters_;
  std::vector<MockWebsiteContent> mock_websites_;
};

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestFiltersListsCallsWithWrongParameters) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();

  // DownloadStats
  VerifyError(
      TriggerCommand("Eyeo.getDownloadStats", {{"configuration", "dummy"}}));

  // SessionStats
  VerifyError(
      TriggerCommand("Eyeo.getSessionStats", {{"configuration", "dummy"}}));

  // FilterLists
  VerifyError(
      TriggerCommand("Eyeo.getFilterLists", {{"configuration", "dummy"}}));
  VerifyError(
      TriggerCommand("Eyeo.addFilterList", {{"configuration", "dummy"}}),
      crdtp::DispatchCode::INVALID_PARAMS);
  VerifyError(
      TriggerCommand("Eyeo.removeFilterList", {{"configuration", "dummy"}}),
      crdtp::DispatchCode::INVALID_PARAMS);
  VerifyError(TriggerCommand("Eyeo.addFilterList", {{"configuration", "dummy"},
                                                    {"url", kCustomList}}));
  VerifyError(
      TriggerCommand("Eyeo.removeFilterList",
                     {{"configuration", "dummy"}, {"url", kCustomList}}));

  // CustomFilters
  VerifyError(
      TriggerCommand("Eyeo.getCustomFilters", {{"configuration", "dummy"}}));
  VerifyError(
      TriggerCommand("Eyeo.addCustomFilter", {{"configuration", "dummy"}}),
      crdtp::DispatchCode::INVALID_PARAMS);
  VerifyError(
      TriggerCommand("Eyeo.removeCustomFilter", {{"configuration", "dummy"}}),
      crdtp::DispatchCode::INVALID_PARAMS);
  VerifyError(
      TriggerCommand("Eyeo.addCustomFilter",
                     {{"configuration", "dummy"}, {"filter", kCustomFilter}}));
  VerifyError(
      TriggerCommand("Eyeo.removeCustomFilter",
                     {{"configuration", "dummy"}, {"filter", kCustomFilter}}));
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestGetConfigurationsCalls) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getConfigurations", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "configurations");
  EXPECT_THAT(actual_values, testing::Contains("adblock: enabled"));
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestGetDownloadStatsInfoCall) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getDownloadStats", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "download_stats");
  EXPECT_FALSE(actual_values.empty());
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestGetSessionStatsInfoCall) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getSessionStats", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "session_stats");
  EXPECT_FALSE(actual_values.empty());
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestFiltersListsCallsWithDefaultConfiguration) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getFilterLists", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(
      actual_values,
      testing::UnorderedElementsAre(
          "https://easylist-downloads.adblockplus.org/easylist.txt",
          "https://easylist-downloads.adblockplus.org/exceptionrules.txt",
          "https://easylist-downloads.adblockplus.org/"
          "abp-filters-anti-cv.txt"));

  EXPECT_TRUE(TriggerCommand("Eyeo.addFilterList", {{"url", kCustomList}}));
  result = TriggerCommand("Eyeo.getFilterLists", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(actual_values, testing::Contains(kCustomList));

  EXPECT_TRUE(TriggerCommand("Eyeo.removeFilterList", {{"url", kCustomList}}));
  result = TriggerCommand("Eyeo.getFilterLists", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(actual_values, testing::Not(testing::Contains(kCustomList)));
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestFiltersListsCallsWithAdblockConfiguration) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result =
      TriggerCommand("Eyeo.getFilterLists", {{"configuration", "adblock"}});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(
      actual_values,
      testing::UnorderedElementsAre(
          "https://easylist-downloads.adblockplus.org/easylist.txt",
          "https://easylist-downloads.adblockplus.org/exceptionrules.txt",
          "https://easylist-downloads.adblockplus.org/"
          "abp-filters-anti-cv.txt"));

  EXPECT_TRUE(
      TriggerCommand("Eyeo.addFilterList",
                     {{"configuration", "adblock"}, {"url", kCustomList}}));
  result =
      TriggerCommand("Eyeo.getFilterLists", {{"configuration", "adblock"}});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(actual_values, testing::Contains(kCustomList));

  EXPECT_TRUE(
      TriggerCommand("Eyeo.removeFilterList",
                     {{"configuration", "adblock"}, {"url", kCustomList}}));
  result =
      TriggerCommand("Eyeo.getFilterLists", {{"configuration", "adblock"}});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "urls");
  EXPECT_THAT(actual_values, testing::Not(testing::Contains(kCustomList)));
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestCustomFiltersCallsWithDefaultConfiguration) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getCustomFilters", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "filters");
  EXPECT_TRUE(actual_values.empty());

  EXPECT_TRUE(
      TriggerCommand("Eyeo.addCustomFilter", {{"filter", kCustomFilter}}));
  result = TriggerCommand("Eyeo.getCustomFilters", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "filters");
  EXPECT_THAT(actual_values, testing::Contains(kCustomFilter));

  EXPECT_TRUE(
      TriggerCommand("Eyeo.removeCustomFilter", {{"filter", kCustomFilter}}));
  result = TriggerCommand("Eyeo.getCustomFilters", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "filters");
  EXPECT_TRUE(actual_values.empty());
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestAllowedDomainsCallsWithDefaultConfiguration) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getAllowedDomains", {});
  ASSERT_TRUE(result);
  auto actual_values = ExtractResponseData(result, "domains");
  EXPECT_TRUE(actual_values.empty());

  EXPECT_TRUE(
      TriggerCommand("Eyeo.addAllowedDomain", {{"domain", kAllowedDomain}}));
  result = TriggerCommand("Eyeo.getAllowedDomains", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "domains");
  EXPECT_THAT(actual_values, testing::Contains(kAllowedDomain));

  EXPECT_TRUE(
      TriggerCommand("Eyeo.removeAllowedDomain", {{"domain", kAllowedDomain}}));
  result = TriggerCommand("Eyeo.getAllowedDomains", {});
  ASSERT_TRUE(result);
  actual_values = ExtractResponseData(result, "domains");
  EXPECT_TRUE(actual_values.empty());
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       TestGetTelemetryDebugInfoCall) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  auto* result = TriggerCommand("Eyeo.getTelemetryDebugInfo", {});
  ASSERT_TRUE(result);
  auto* actual_values = result->FindString("telemetry_ping");
  EXPECT_FALSE(actual_values->empty());
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       ConfigurationEventsTest) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  SendCommandSync("Eyeo.enable");
  // FilterLists
  EXPECT_TRUE(TriggerCommand("Eyeo.addFilterList", {{"url", kCustomList}}));
  WaitForEventNotification("Eyeo.filterListsChanged", "urls");
  EXPECT_TRUE(TriggerCommand("Eyeo.removeFilterList", {{"url", kCustomList}}));
  WaitForEventNotification("Eyeo.filterListsChanged", "urls");

  // CustomFilters
  EXPECT_TRUE(
      TriggerCommand("Eyeo.addCustomFilter", {{"filter", kCustomFilter}}));
  WaitForEventNotification("Eyeo.customFiltersChanged", "filters");
  EXPECT_TRUE(
      TriggerCommand("Eyeo.removeCustomFilter", {{"filter", kCustomFilter}}));
  WaitForEventNotification("Eyeo.customFiltersChanged", "filters");
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       PageAllowedFilterHitEventsTest) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  SendCommandSync("Eyeo.enable");
  AddCustomFilters({"@@||example.com^$document,domain=example.com"});
  RegisterHtmlContent("/test_page.html", R"(
    <html>
    <head>
    <title>Test page</title>
    </head>
    <body>
    </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  WaitForFilterHitEvent("Eyeo.pageAllowed",
                        GURL{"https://example.com/test_page.html"});
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       RequestAllowedFilterHitEventsTest) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  SendCommandSync("Eyeo.enable");
  AddCustomFilters({"allowed_resource.png", "@@allowed_resource.png"});
  RegisterHtmlContent("/test_page.html", R"(
    <html>
    <head>
    <title>Test page</title>
    </head>
    <body>
    <img src="allowed_resource.png">
    </body>
    </html>
  )");
  NavigateToPage(GURL("https://example.com/test_page.html"));
  WaitForFilterHitEvent("Eyeo.requestAllowed",
                        GURL{"https://example.com/allowed_resource.png"});
}

IN_PROC_BROWSER_TEST_F(DevToolsEyeoHandlerBrowserTest,
                       RequestBlockedFilterHitEventsTest) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL("about:blank")));
  Attach();
  SendCommandSync("Eyeo.enable");
  AddCustomFilters({"blocked_resource.png"});
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
  WaitForFilterHitEvent("Eyeo.requestBlocked",
                        GURL{"https://example.com/blocked_resource.png"});
}

}  // namespace content
