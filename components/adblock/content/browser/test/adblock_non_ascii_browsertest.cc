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

#include "base/strings/stringprintf.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockNonASCIIBrowserTest : public AdblockBrowserTestBase {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("xn----dtbfdbwspgnceulm.xn--p1ai", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  std::string ExecuteScriptAndExtractString(const std::string& js_code) {
    return content::EvalJs(web_contents(), js_code).ExtractString();
  }
};

IN_PROC_BROWSER_TEST_F(AdblockNonASCIIBrowserTest, BlockNonASCII) {
  static constexpr char visibility_check[] =
      "getComputedStyle(document.getElementsByClassName(\"форум\")[0]).display"
      " === '%s'";

  ASSERT_TRUE(content::NavigateToURL(
      shell(),
      embedded_test_server()->GetURL("форум-трейдеров.рф", "/non-ascii.html")));
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(visibility_check, "block").c_str()));

  SetFilters({"xn----dtbfdbwspgnceulm.xn--p1ai##.форум"});
  ASSERT_TRUE(content::NavigateToURL(
      shell(),
      embedded_test_server()->GetURL("форум-трейдеров.рф", "/non-ascii.html")));
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(visibility_check, "none").c_str()));
}

}  // namespace adblock
