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
#include "base/run_loop.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
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

class AdblockServiceWorkersBrowserTest : public AdblockBrowserTestBase {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Note, fetch_from_service_worker.js and fetch_from_service_worker.html are
    // also available in content/test/data. This could be a content_browsertest,
    // probably.
    embedded_test_server()->ServeFilesFromSourceDirectory("chrome/test/data");
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&AdblockServiceWorkersBrowserTest::RequestHandler,
                            base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url == "/requested_path") {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content("fetch completed");
      http_response->set_content_type("text/plain");
      http_response->AddCustomHeader("test_header_key", "test_header_value");
      return std::move(http_response);
    }
    return nullptr;
  }

  void AddAllowedDomain(std::string domain) {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser_context())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    adblock_configuration->AddAllowedDomain(domain);
  }

  GURL GetPageUrl() {
    // Reusing an existing test page to avoid creating a new one.
    // This page exposes a setup() function to register a service worker
    // and a fetch_from_service_worker() function to perform a network fetch
    // by sending an internal message to the service worker.
    return embedded_test_server()->GetURL(
        "/service_worker/fetch_from_service_worker.html");
  }
};

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest, NoBlockingByDefault) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));

  // "fetch completed" is returned by our RequestHandler.
  EXPECT_EQ("fetch completed",
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       ResourceBlockedByCustomFilter) {
  SetFilters({"*requested_path"});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));

  EXPECT_EQ("TypeError: Failed to fetch",
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       ResourceBlockedByHeaderFilter) {
  // A header filter must have a pattern that matches the request URL.
  // The pattern is the host name of the test server.
  const auto host_port_pair = embedded_test_server()->host_port_pair();
  const std::string header_filter =
      host_port_pair.host() + "$header=test_header_key=test_header_value";
  SetFilters({header_filter});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));

  EXPECT_EQ("TypeError: Failed to fetch",
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       ResourceRedirectedByRewriteFilter) {
  const auto host_port_pair = embedded_test_server()->host_port_pair();
  const auto host = host_port_pair.host();
  const auto filter =
      "||" + host +
      "*/requested_path$rewrite=abp-resource:blank-html,domain=" + host;
  SetFilters({filter});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));

  // The service worker fetches /requested_path, but the response is a blank
  // HTML document - as redirected by the filter.
  // Seems that JS escapes '<' characters with '\u003C' in the response,
  // possibly as a side effect or safety measure related to eval().
  const std::string_view blank_html =
      "\u003C!DOCTYPE "
      "html>\u003Chtml>\u003Chead>\u003C/head>\u003Cbody>\u003C/body>\u003C/"
      "html>";
  EXPECT_EQ(blank_html,
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       ResourceAllowedByResourceSpecificAllowingRule) {
  SetFilters({"*resource.png"});
  SetFilters({"@@*resource.png"});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));
  EXPECT_EQ("fetch completed",
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       ResourceAllowedByOrigin) {
  SetFilters({"*resource.png"});
  AddAllowedDomain(GetPageUrl().host());

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));
  EXPECT_EQ("fetch completed",
            content::EvalJs(web_contents(),
                            "fetch_from_service_worker('/requested_path');"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest, EntireScriptBlocked) {
  SetFilters({"*fetch_from_service_worker.js"});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  // The content of the service worker script is not allowed to download, the
  // page cannot register the service worker.
  const auto setup_result = content::EvalJs(web_contents(), "setup();");
  // The error message will indicate blocking succeeded.
  EXPECT_NE(
      setup_result.error.find("TypeError: Failed to register a ServiceWorker"),
      std::string::npos);
  EXPECT_NE(setup_result.error.find("error occurred when fetching the script"),
            std::string::npos);
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       EntireScriptAllowedByOrigin) {
  SetFilters({"*fetch_from_service_worker.js"});
  AddAllowedDomain(GetPageUrl().host());

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));
}

IN_PROC_BROWSER_TEST_F(AdblockServiceWorkersBrowserTest,
                       EntireScriptAllowedBySpecificAllowingRule) {
  SetFilters({"*fetch_from_service_worker.js"});
  SetFilters({"@@*fetch_from_service_worker.js"});

  ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  EXPECT_EQ("ready", content::EvalJs(web_contents(), "setup();"));
}

}  // namespace adblock
