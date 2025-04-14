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

#include "base/strings/utf_string_conversions.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/install_default_websocket_handlers.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockContentBrowserClientBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockContentBrowserClientBrowserTest()
      : ws_server_(net::EmbeddedTestServer::TYPE_HTTP) {
    net::test_server::InstallDefaultWebSocketHandlers(
        &ws_server_, /*serve_websocket_test_data=*/true);
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    watcher_ = std::make_unique<content::TitleWatcher>(web_contents(), u"PASS");
    watcher_->AlsoWaitForTitle(u"FAIL");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        net::GetWebSocketTestDataDirectory());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override { watcher_.reset(); }

  void NavigateToHTTP(const std::string& path) {
    // Visit a HTTP page for testing.
    const GURL url = ws_server_.GetURL(path);
    ASSERT_TRUE(url.SchemeIs("http"));
    ASSERT_TRUE(content::NavigateToURL(shell(), url));
  }

  std::string WaitAndGetTitle() {
    return base::UTF16ToUTF8(watcher_->WaitAndGetTitle());
  }

  net::EmbeddedTestServer ws_server_;

 private:
  std::unique_ptr<content::TitleWatcher> watcher_;
};

IN_PROC_BROWSER_TEST_F(AdblockContentBrowserClientBrowserTest,
                       WebSocketConnectionNotIntercepted) {
  // Launch a WebSocket server.
  ASSERT_TRUE(ws_server_.Start());

  // Disable ad-filtering.
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  adblock_configuration->SetEnabled(false);

  NavigateToHTTP("/split_packet_check.html");

  // WebSocket connected.
  EXPECT_EQ("PASS", WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(AdblockContentBrowserClientBrowserTest,
                       WebSocketConnectionInterceptedButNotBlocked) {
  // Launch a WebSocket server.
  ASSERT_TRUE(ws_server_.Start());

  NavigateToHTTP("/split_packet_check.html");

  // WebSocket connected, there were no blocking filters.
  EXPECT_EQ("PASS", WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(AdblockContentBrowserClientBrowserTest,
                       WebSocketConnectionInterceptedAndBlocked) {
  // Launch a WebSocket server.
  ASSERT_TRUE(ws_server_.Start());

  // Intercept WebSocket and block connection via a filter.
  SetFilters({"*$websocket"});

  NavigateToHTTP("/split_packet_check.html");

  // WebSocket did not connect.
  EXPECT_EQ("FAIL", WaitAndGetTitle());
}

}  // namespace adblock
