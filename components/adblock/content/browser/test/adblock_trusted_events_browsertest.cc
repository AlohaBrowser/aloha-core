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

#include <cstdint>

#include "content/public/common/isolated_world_ids.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockTrustedEventsTest
    : public content::ContentBrowserTest,
      public testing::WithParamInterface<content::IsolatedWorldIDs> {
 public:
  void SetUpOnMainThread() override {
    content::ContentBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule(kTestDomain, "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  GURL GetPageUrl() {
    return embedded_test_server()->GetURL(kTestDomain, "/trusted_events.html");
  }

  void NavigateToPage() {
    ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
    static std::string script =
        R"(
        function triggerJsClick() {
          document.getElementById("test_button").click();
          return false;
        }
        function dispatchJsClickEvent() {
          let clickEvent = new Event('click');
          document.getElementById("test_button").dispatchEvent(clickEvent);
          return false;
        }
        function dispatchJsMouseoverEvent() {
          let mouseoverEvent = new Event('mouseover');
          document.getElementById("test_button").dispatchEvent(mouseoverEvent);
          return false;
        }
        function reset(value) {
          document.getElementById("result").value = (value ? value : "");
          return false;
        }
        document.getElementById("test_button").addEventListener("click", (event) => reset(String(event.isTrusted)));
        document.getElementById("test_button").addEventListener("mouseover", (event) => reset(String(event.isTrusted)));
        document.getElementById("reset").onclick = reset;
        document.getElementById("triggerJsClick").onclick = triggerJsClick;
        document.getElementById("dispatchJsClickEvent").onclick = dispatchJsClickEvent;
      )";
    (void)(content::EvalJs(
        shell(), script, content::EvalJsOptions::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        GetParam()));
  }

  bool GetIsTrustedValue() {
    auto value =
        content::EvalJs(shell(), "document.getElementById('result').value")
            .ExtractString();
    CHECK(value == "true" || value == "false");
    return value == "true";
  }

  void TriggerEvent(const std::string& js_code) {
    EXPECT_EQ(false, content::EvalJs(
                         shell(), js_code,
                         content::EvalJsOptions::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         GetParam()));
  }

  bool IsAdblockWorld() {
    return GetParam() == content::IsolatedWorldIDs::ISOLATED_WORLD_ID_ADBLOCK;
  }

 private:
  static constexpr char kTestDomain[] = "test.org";
};

IN_PROC_BROWSER_TEST_P(AdblockTrustedEventsTest,
                       VerifyClickEventTrustedInAdblockIsolatedWorld) {
  NavigateToPage();
  TriggerEvent("triggerJsClick();");
  // Should be trusted only in adblock isolated world
  EXPECT_EQ(IsAdblockWorld(), GetIsTrustedValue());
}

IN_PROC_BROWSER_TEST_P(AdblockTrustedEventsTest,
                       VerifyDispatchClickEventTrustedInAdblockIsolatedWorld) {
  NavigateToPage();
  TriggerEvent("dispatchJsClickEvent();");
  // Should be trusted only in adblock isolated world
  EXPECT_EQ(IsAdblockWorld(), GetIsTrustedValue());
}

IN_PROC_BROWSER_TEST_P(
    AdblockTrustedEventsTest,
    VerifyDispatchMouseoverEventTrustedInAdblockIsolatedWorld) {
  NavigateToPage();
  TriggerEvent("dispatchJsMouseoverEvent();");
  // Should be trusted only in adblock isolated world
  EXPECT_EQ(IsAdblockWorld(), GetIsTrustedValue());
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockTrustedEventsTest,
    testing::Values(content::IsolatedWorldIDs::ISOLATED_WORLD_ID_GLOBAL,
                    content::IsolatedWorldIDs::ISOLATED_WORLD_ID_ADBLOCK,
                    content::IsolatedWorldIDs::ISOLATED_WORLD_ID_CONTENT_END));

}  // namespace adblock
