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

#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/api/adblock_private/adblock_private_apitest_base.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "extensions/browser/background_script_executor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace extensions {

// Here are extension API tests for PageAllowed event and popup
// events and stats, which are difficult to implement like other
// tests in AdblockPrivateAPI class (purely in JS in test.js file).
// Tests here require a background page to run script code.
class AdblockPrivateApiBackgroundPageTest
    : public AdblockPrivateApiTestBase,
      public testing::WithParamInterface<
          std::tuple<AdblockPrivateApiTestBase::EyeoExtensionApi,
                     AdblockPrivateApiTestBase::Mode>> {
 public:
  AdblockPrivateApiBackgroundPageTest() {}
  ~AdblockPrivateApiBackgroundPageTest() override = default;
  AdblockPrivateApiBackgroundPageTest(
      const AdblockPrivateApiBackgroundPageTest&) = delete;
  AdblockPrivateApiBackgroundPageTest& operator=(
      const AdblockPrivateApiBackgroundPageTest&) = delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    AdblockPrivateApiTestBase::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    // Map example.com to localhost.
    host_resolver()->AddRule("example.com", "127.0.0.1");

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    https_test_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_test_server_->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_test_server_->Start());

    ASSERT_TRUE(StartEmbeddedTestServer());
    adblock::SubscriptionServiceFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->GetFilteringConfiguration(adblock::kAdblockFilteringConfigurationName)
        ->RemoveCustomFilter(adblock::kAllowlistEverythingFilter);

    extension_ = LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                               {.allow_in_incognito = IsIncognito()});
    ASSERT_TRUE(extension_);
  }

  bool IsOldApi() { return std::get<0>(GetParam()) == EyeoExtensionApi::Old; }

  std::string GetApiEndpoint() override {
    return IsOldApi() ? "adblockPrivate" : "eyeoFilteringPrivate";
  }

  bool IsIncognito() override {
    return std::get<1>(GetParam()) ==
           AdblockPrivateApiBackgroundPageTest::Mode::Incognito;
  }

  void ExecuteScript(const std::string& js_code) const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetWebContentsAt(0);
    ASSERT_TRUE(content::ExecJs(web_contents->GetPrimaryMainFrame(), js_code));
  }

  int ExecuteScriptAndGetInt(const std::string& extension_id,
                             const std::string& script) {
    return ExtensionApiTest::ExecuteScriptInBackgroundPage(extension_id, script)
        .GetInt();
  }

  void SetupApiObjectAndMethods(const std::string& extension_id) {
    std::string setup_script;
    if (IsOldApi()) {
      setup_script =
          "let apiObject = chrome.adblockPrivate;"
          "let sessionAllowedCount = 'getSessionAllowedAdsCount';"
          "let sessionBlockedCount = 'getSessionBlockedAdsCount';"
          "let onAllowedEvent = 'onAdAllowed';"
          "let onBlockedEvent = 'onAdBlocked';"
          "chrome.test.sendScriptResult(0);";
    } else {
      setup_script =
          "let apiObject = chrome.eyeoFilteringPrivate;"
          "let sessionAllowedCount = 'getSessionAllowedRequestsCount';"
          "let sessionBlockedCount = 'getSessionBlockedRequestsCount';"
          "let onAllowedEvent = 'onRequestAllowed';"
          "let onBlockedEvent = 'onRequestBlocked';"
          "chrome.test.sendScriptResult(0);";
    }
    ExecuteScriptAndGetInt(extension_id, setup_script);
  }

  const ExtensionId& GetExtensionId() const { return extension_->id(); }

  std::unique_ptr<net::EmbeddedTestServer> https_test_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  raw_ptr<const Extension, DanglingUntriaged> extension_;
};

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PageAllowedEvents) {
  constexpr char kSetListenersScript[] = R"(
    let testData = {};
    testData.pageAllowedCount = 0;
    apiObject.onPageAllowed.addListener(function(e) {
      if (!e.url.endsWith('test.html')) {
        return;
      }
      testData.pageAllowedCount = testData.pageAllowedCount + 1;
    });
    chrome.test.sendScriptResult(0);
  )";

  constexpr char kReadCountersScript[] = R"(
    var intervalId = setInterval(function() {
      if (testData.pageAllowedCount == %d) {
        if (intervalId) {
          clearInterval(intervalId);
          intervalId = null;
        }
        chrome.test.sendScriptResult(testData.pageAllowedCount);
      }
    }, 100);
  )";

  constexpr char kAllowDomainScript[] = R"(
    apiObject.addAllowedDomain(%s'example.com');
    chrome.test.sendScriptResult(0);
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");

  SetupApiObjectAndMethods(GetExtensionId());

  ExecuteScriptAndGetInt(GetExtensionId(), kSetListenersScript);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(0,
            ExecuteScriptAndGetInt(GetExtensionId(),
                                   base::StringPrintf(kReadCountersScript, 0)));

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kAllowDomainScript, IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(1,
            ExecuteScriptAndGetInt(GetExtensionId(),
                                   base::StringPrintf(kReadCountersScript, 1)));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PageAllowedStats) {
  constexpr char kReadAllowedStatsScript[] = R"(
    var intervalId = setInterval(function() {
      apiObject[sessionAllowedCount](function(sessionStats) {
        let count = 0;
        for (const entry of sessionStats) {
          if (entry.url === 'adblock:custom') {
            count = entry.count;
          }
        }
        if (%d == 0 || count == %d) {
          if (intervalId) {
            clearInterval(intervalId);
            intervalId = null;
          }
          chrome.test.sendScriptResult(count);
        }
      });
    }, 100);
  )";

  constexpr char kAllowDomainScript[] = R"(
    apiObject.addAllowedDomain(%s'example.com');
    chrome.test.sendScriptResult(0);
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");

  SetupApiObjectAndMethods(GetExtensionId());

  int initial_value = ExecuteScriptAndGetInt(
      GetExtensionId(), base::StringPrintf(kReadAllowedStatsScript, 0, 0));

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kAllowDomainScript, IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  EXPECT_EQ(
      initial_value + 1,
      ExecuteScriptAndGetInt(
          GetExtensionId(), base::StringPrintf(kReadAllowedStatsScript, 1, 1)));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PopupEvents) {
  constexpr char kSetListenersScript[] = R"(
    let testData = {};
    testData.popupBlockedCount = 0;
    testData.popupAllowedCount = 0;
    apiObject.onPopupAllowed.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupAllowedCount = testData.popupAllowedCount + 1;
    });
    apiObject.onPopupBlocked.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupBlockedCount = testData.popupBlockedCount + 1;
    });
    chrome.test.sendScriptResult(0);
  )";

  constexpr char verify_stats_script_tmpl[] = R"(
        var intervalId = setInterval(function() {
          let result = testData.%s;
          if (result == %d) {
            if (intervalId) {
              clearInterval(intervalId);
              intervalId = null;
            }
            chrome.test.sendScriptResult(result);
          }
        }, 100);
  )";

  auto read_allowed_stats_script = [verify_stats_script_tmpl](int expected) {
    std::string script = base::StringPrintf(verify_stats_script_tmpl,
                                            "popupAllowedCount", expected);
    return script;
  };

  auto read_blocked_stats_script = [verify_stats_script_tmpl](int expected) {
    std::string script = base::StringPrintf(verify_stats_script_tmpl,
                                            "popupBlockedCount", expected);
    return script;
  };

  constexpr char kBlockPopupScript[] = R"(
    apiObject.addCustomFilter(%s'some-popup.html^$popup');
    chrome.test.sendScriptResult(0);
  )";

  constexpr char kAllowPopupScript[] = R"(
    apiObject.addCustomFilter(%s'@@some-popup.html^$popup');
    chrome.test.sendScriptResult(0);
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");
  constexpr char kOpenPopupScript[] =
      "document.getElementById('popup_id').click()";
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  SetupApiObjectAndMethods(GetExtensionId());

  ExecuteScriptAndGetInt(GetExtensionId(), kSetListenersScript);

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kBlockPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  EXPECT_EQ(0, ExecuteScriptAndGetInt(GetExtensionId(),
                                      read_allowed_stats_script(0)));
  EXPECT_EQ(1, ExecuteScriptAndGetInt(GetExtensionId(),
                                      read_blocked_stats_script(1)));

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kAllowPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  EXPECT_EQ(1, ExecuteScriptAndGetInt(GetExtensionId(),
                                      read_allowed_stats_script(1)));
  EXPECT_EQ(1, ExecuteScriptAndGetInt(GetExtensionId(),
                                      read_blocked_stats_script(1)));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PopupStats) {
  constexpr char verify_stats_script_tmpl[] = R"(
    var intervalId = setInterval(function() {
      apiObject[%s](function(sessionStats) {
        let count = 0;
        for (const entry of sessionStats) {
          if (entry.url === 'adblock:custom') {
            count = entry.count;
          }
        }
        if (%d == 0 || count == %d) {
          if (intervalId) {
            clearInterval(intervalId);
            intervalId = null;
          }
          chrome.test.sendScriptResult(count);
        }
      }
    )}, 100);
  )";

  auto read_allowed_stats_script = [verify_stats_script_tmpl](int expected) {
    std::string script = base::StringPrintf(
        verify_stats_script_tmpl, "sessionAllowedCount", expected, expected);
    return script;
  };

  auto read_blocked_stats_script = [verify_stats_script_tmpl](int expected) {
    std::string script = base::StringPrintf(
        verify_stats_script_tmpl, "sessionBlockedCount", expected, expected);
    return script;
  };

  constexpr char kBlockPopupScript[] = R"(
    apiObject.addCustomFilter(%s'some-popup.html^$popup');
    chrome.test.sendScriptResult(0);
  )";

  constexpr char kAllowPopupScript[] = R"(
    apiObject.addCustomFilter(%s'@@some-popup.html^$popup');
    chrome.test.sendScriptResult(0);
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");
  constexpr char kOpenPopupScript[] =
      "document.getElementById('popup_id').click()";
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  SetupApiObjectAndMethods(GetExtensionId());

  int initial_allowed_value =
      ExecuteScriptAndGetInt(GetExtensionId(), read_allowed_stats_script(0));
  int initial_blocked_value =
      ExecuteScriptAndGetInt(GetExtensionId(), read_blocked_stats_script(0));

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kBlockPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(initial_blocked_value + 1,
            ExecuteScriptAndGetInt(
                GetExtensionId(),
                read_blocked_stats_script(initial_blocked_value + 1)));

  ExecuteScriptAndGetInt(
      GetExtensionId(),
      base::StringPrintf(kAllowPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(initial_allowed_value + 1,
            ExecuteScriptAndGetInt(
                GetExtensionId(),
                read_allowed_stats_script(initial_allowed_value + 1)));
  ASSERT_EQ(initial_blocked_value + 1,
            ExecuteScriptAndGetInt(
                GetExtensionId(),
                read_blocked_stats_script(initial_blocked_value + 1)));
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockPrivateApiBackgroundPageTest,
    testing::Combine(
        testing::Values(
            AdblockPrivateApiBackgroundPageTest::EyeoExtensionApi::Old,
            AdblockPrivateApiBackgroundPageTest::EyeoExtensionApi::New),
        testing::Values(AdblockPrivateApiBackgroundPageTest::Mode::Normal,
                        AdblockPrivateApiBackgroundPageTest::Mode::Incognito)));

}  // namespace extensions
