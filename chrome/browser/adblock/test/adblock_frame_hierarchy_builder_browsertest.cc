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

#include <algorithm>
#include <vector>

#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_upgrades_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/blocked_content/popup_blocker_tab_helper.h"
#include "components/embedder_support/switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

namespace {
class TabAddedRemovedObserver : public TabStripModelObserver {
 public:
  explicit TabAddedRemovedObserver(TabStripModel* tab_strip_model) {
    tab_strip_model->AddObserver(this);
  }

  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override {
    if (change.type() == TabStripModelChange::kInserted) {
      inserted_ = true;
      return;
    }
    if (change.type() == TabStripModelChange::kRemoved) {
      EXPECT_TRUE(inserted_);
      removed_ = true;
      loop_.Quit();
      return;
    }
    NOTREACHED();
  }

  void Wait() {
    if (inserted_ && removed_) {
      return;
    }
    loop_.Run();
  }

 private:
  bool inserted_ = false;
  bool removed_ = false;
  base::RunLoop loop_;
};
}  // namespace

class ResourceClassificationRunnerObserver
    : public ResourceClassificationRunner::Observer {
 public:
  ~ResourceClassificationRunnerObserver() override {
    VerifyNoUnexpectedNotifications();
  }
  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {
    if (match_result == FilterMatchResult::kAllowRule) {
      allowed_ads_notifications.push_back(url);
    } else {
      blocked_ads_notifications.push_back(url);
    }
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {
    allowed_pages_notifications.push_back(url);
  }

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {
    auto& list = (match_result == FilterMatchResult::kBlockRule
                      ? blocked_popups_notifications
                      : allowed_popups_notifications);
    auto it = std::find(list.begin(), list.end(), url.ExtractFileName());
    ASSERT_FALSE(it == list.end())
        << "Path " << url.ExtractFileName() << " not on list";
    list.erase(it);
    if (popup_notifications_run_loop_ && allowed_popups_notifications.empty() &&
        blocked_popups_notifications.empty()) {
      popup_notifications_run_loop_->Quit();
    }
  }

  void OnPageElementMatched(
      const std::string& selector,
      adblock::ElementHideAction action,
      content::RenderFrameHost* render_frame_host) override {
    auto it = std::find(hidden_notifications.begin(),
                        hidden_notifications.end(), selector);
    ASSERT_FALSE(it == hidden_notifications.end())
        << "Selector " << selector << " not on list";
    hidden_notifications.erase(it);
    if (hidden_notifications.empty() && eh_quit_closure_) {
      eh_quit_closure_.Run();
    }
  }

  void VerifyNotificationSent(std::string_view path, std::vector<GURL>& list) {
    auto it = std::ranges::find(list, path, &GURL::ExtractFileName);
    ASSERT_FALSE(it == list.end()) << "Path " << path << " not on list";
    // Remove expected notifications so that we can verify there are no
    // unexpected notifications left by the end of each test.
    list.erase(it);
  }

  void VerifyNoUnexpectedNotifications() {
    EXPECT_TRUE(blocked_ads_notifications.empty());
    EXPECT_TRUE(allowed_ads_notifications.empty());
    EXPECT_TRUE(blocked_popups_notifications.empty());
    EXPECT_TRUE(allowed_popups_notifications.empty());
    EXPECT_TRUE(allowed_pages_notifications.empty());
    EXPECT_TRUE(hidden_notifications.empty());
  }

  std::vector<GURL> blocked_ads_notifications;
  std::vector<GURL> allowed_ads_notifications;
  std::vector<GURL> allowed_pages_notifications;
  std::vector<std::string> blocked_popups_notifications;
  std::vector<std::string> allowed_popups_notifications;
  std::vector<std::string> hidden_notifications;
  std::unique_ptr<base::RunLoop> popup_notifications_run_loop_;
  base::RepeatingClosure eh_quit_closure_;
};

// Simulated setup:
// http://outer.com/outermost_frame.html
//   has an iframe: http://middle.com/middle_frame.html
//     has an iframe: http://inner.com/innermost_frame.html
//       has a subresource http://inner.com/resource.png
//
// All of these files are in components/test/data/adblock. Cross-domain
// distribution is simulated via SetupCrossSiteRedirector. innermost_frame.html
// reports whether resource.png is visible via window.top.postMessage to
// outermost_frame.html, which stores a global subresource_visible JS variable.
class AdblockFrameHierarchyBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    content::SetupCrossSiteRedirector(embedded_test_server());
    AllowHttpForHostnamesForTesting(
        {"outer.com", "inner.com", "middle.com"},
        browser()->profile()->GetOriginalProfile()->GetPrefs());
    ASSERT_TRUE(embedded_test_server()->Start());
    auto* classification_runner =
        ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile());
    classification_runner->AddObserver(&observer);
  }

  void TearDownOnMainThread() override {
    auto* classification_runner =
        ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile());
    classification_runner->RemoveObserver(&observer);
    InProcessBrowserTest::TearDownOnMainThread();
  }

  bool IncognitoMode() { return GetParam(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(embedder_support::kDisablePopupBlocking);
    if (IncognitoMode()) {
      command_line->AppendSwitch("incognito");
    }
  }

  void SetFilters(std::vector<std::string> filters) {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    adblock_configuration->RemoveCustomFilter(kAllowlistEverythingFilter);
    for (auto& filter : filters) {
      adblock_configuration->AddCustomFilter(filter);
    }
  }

  void NavigateToOutermostFrame() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(
                       "/cross-site/outer.com/outermost_frame.html")));
  }

  void NavigateToOutermostFrameWithAboutBlank() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL(
            "/cross-site/outer.com/outermost_frame_with_about_blank.html")));
  }

  void NavigateToPopupParentFrameAndWaitForNotifications() {
    observer.popup_notifications_run_loop_ = std::make_unique<base::RunLoop>();
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(
                       "/cross-site/outer.com/popup_parent.html")));
    if (!(observer.allowed_popups_notifications.empty() &&
          observer.blocked_popups_notifications.empty())) {
      observer.popup_notifications_run_loop_->Run();
    }
  }

  void VerifyTargetResourceShown(bool expected_visible,
                                 const char* visibility_param) {
    // Since in one test we dynamically load (write) `about:blank` iframe after
    // parent page is loaded, we need to have some wait & poll mechanism to wait
    // until such a frame (and its resources) is completely loaded by JS.
    std::string script = base::StringPrintf(
        R"(
      (async () => {
        let count = 10;
        let visibility_param = '%s';
        function waitFor(condition) {
          const poll = resolve => {
            if(condition() || !count--) resolve();
            else setTimeout(_ => poll(resolve), 200);
          }
          return new Promise(poll);
        }
        // Waits up to 2 seconds
        await waitFor(_ => window.subresource_visibility[visibility_param] === %s);
        return window.subresource_visibility[visibility_param] === false;
     })()
     )",
        visibility_param, expected_visible ? "false" : "true");
    EXPECT_EQ(
        expected_visible,
        content::EvalJs(browser()->tab_strip_model()->GetActiveWebContents(),
                        script));
  }

  void VerifyTargetResourceBlockingStatus(bool expected_visible) {
    VerifyTargetResourceShown(expected_visible, "is_blocked");
  }

  void VerifyTargetResourceHidingStatus(bool expected_visible) {
    VerifyTargetResourceShown(expected_visible, "is_hidden");
  }

  int NumberOfOpenTabs() { return browser()->tab_strip_model()->GetTabCount(); }

  ResourceClassificationRunnerObserver observer;
};

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       SubresourceShownWithNoFilters) {
  SetFilters({});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(true);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest, SubresourceBlocked) {
  SetFilters({"/resource.png"});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(false);
  observer.VerifyNotificationSent("resource.png",
                                  observer.blocked_ads_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaInnerFrame) {
  SetFilters({"/resource.png", "@@||inner.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(true);
  observer.VerifyNotificationSent("resource.png",
                                  observer.allowed_ads_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaMiddleFrame) {
  SetFilters({"/resource.png", "@@||middle.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(true);
  observer.VerifyNotificationSent("resource.png",
                                  observer.allowed_ads_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaOutermostFrame) {
  SetFilters({"/resource.png", "@@||outer.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(true);
  observer.VerifyNotificationSent("resource.png",
                                  observer.allowed_ads_notifications);
  observer.VerifyNotificationSent("outermost_frame.html",
                                  observer.allowed_pages_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       SubresourceBlockedWhenInvalidAllowRule) {
  SetFilters({"/resource.png", "@@||bogus.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceBlockingStatus(false);
  observer.VerifyNotificationSent("resource.png",
                                  observer.blocked_ads_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       DISABLED_PopupHandledByChromiumWithoutFilters) {
  // Without any popup-specific filters, blocking popups is handed over to
  // Chromium, which has it's own heuristics that are not based on filters.
  SetFilters({});
  NavigateToPopupParentFrameAndWaitForNotifications();
  // The popup was not opened:
  EXPECT_EQ(1, NumberOfOpenTabs());
  // Because Chromium's built-in popup blocker stopped it:
  EXPECT_EQ(1u, blocked_content::PopupBlockerTabHelper::FromWebContents(
                    browser()->tab_strip_model()->GetActiveWebContents())
                    ->GetBlockedPopupsCount());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest, PopupBlockedByFilter) {
  SetFilters({"http://inner.com*/popup.html$popup"});
  observer.blocked_popups_notifications.emplace_back("popup.html");
  TabAddedRemovedObserver observer(browser()->tab_strip_model());
  NavigateToPopupParentFrameAndWaitForNotifications();
  observer.Wait();
  EXPECT_EQ(1, NumberOfOpenTabs());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest, PopupAllowedByFilter) {
  SetFilters({"http://inner.com*/popup.html$popup",
              "@@http://inner.com*/popup.html$popup"});
  observer.allowed_popups_notifications.emplace_back("popup.html");
  NavigateToPopupParentFrameAndWaitForNotifications();
  // Popup was allowed to open in a new tab
  EXPECT_EQ(2, NumberOfOpenTabs());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       PopupAllowedByDomainSpecificFilter) {
  // The frame that wants to open the popup is hosted on middle.com.
  // The $popup allow rule applies to that frame.
  SetFilters({"http://inner.com*/popup.html$popup",
              "@@http://inner.com*/popup.html$popup,domain=middle.com"});
  observer.allowed_popups_notifications.emplace_back("popup.html");
  NavigateToPopupParentFrameAndWaitForNotifications();
  // Popup was allowed to open in a new tab
  EXPECT_EQ(2, NumberOfOpenTabs());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       PopupNotAllowedByDomainSpecificFilter) {
  // The frame that wants to open the popup is hosted on middle.com.
  // The $popup allow rule does not apply because it is specific to outer.com.
  // outer.com is not the frame that is opening the popup.
  SetFilters({"http://inner.com*/popup.html$popup",
              "@@http://inner.com*/popup.html$popup,domain=outer.com"});
  observer.blocked_popups_notifications.emplace_back("popup.html");
  TabAddedRemovedObserver observer(browser()->tab_strip_model());
  NavigateToPopupParentFrameAndWaitForNotifications();
  observer.Wait();
  EXPECT_EQ(1, NumberOfOpenTabs());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       PopupAllowedByParentDocument) {
  // The outermost frame has a blanket allowing rule of $document type.
  SetFilters({"http://inner.com*/popup.html$popup",
              "@@||outer.com^$document,domain=outer.com"});
  observer.allowed_popups_notifications.emplace_back("popup.html");
  NavigateToPopupParentFrameAndWaitForNotifications();
  // Popup was allowed to open in a new tab
  EXPECT_EQ(2, NumberOfOpenTabs());
  observer.VerifyNotificationSent("popup_parent.html",
                                  observer.allowed_pages_notifications);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest,
                       PopupAllowedByIntermediateParentDocument) {
  // The middle frame has a blanket allowing rule of $document type.
  SetFilters({"http://inner.com*/popup.html$popup",
              "@@||middle.com^$document,domain=outer.com"});
  observer.allowed_popups_notifications.emplace_back("popup.html");
  NavigateToPopupParentFrameAndWaitForNotifications();
  // Popup was allowed to open in a new tab
  EXPECT_EQ(2, NumberOfOpenTabs());
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest, BlankFrameHiding) {
  SetFilters({"##.about_blank_div"});
  NavigateToOutermostFrameWithAboutBlank();
  std::string script = R"(
    function writeIframe() {
      let frameDocument = document.getElementById("about_blank").contentWindow.document;
      frameDocument.open("text/html");
      frameDocument.write(`
        <html>
          <body>
            <div class="about_blank_div">
              <iframe id='middle_frame' src='/cross-site/middle.com/middle_frame.html' width='400'></iframe>
            </div>
          </body>
        </html>`);
      frameDocument.close();
    }
    if (document.readyState == "complete") {
      writeIframe();
    } else {
      document.getElementById("about_blank").addEventListener("load", writeIframe);
    }
  )";
  observer.hidden_notifications.emplace_back(
      "type:div;className:about_blank_div");
  EXPECT_TRUE(content::ExecJs(
      browser()->tab_strip_model()->GetActiveWebContents(), script));
  base::RunLoop run_loop;
  observer.eh_quit_closure_ = run_loop.QuitClosure();
  std::move(run_loop).Run();
  VerifyTargetResourceHidingStatus(false);
  SetFilters({"@@^eyeo=true$document"});
  NavigateToOutermostFrameWithAboutBlank();
  EXPECT_TRUE(content::ExecJs(
      browser()->tab_strip_model()->GetActiveWebContents(), script));
  VerifyTargetResourceHidingStatus(true);
}

IN_PROC_BROWSER_TEST_P(AdblockFrameHierarchyBrowserTest, BlankFrameBlocking) {
  SetFilters({"/resource.png"});
  NavigateToOutermostFrameWithAboutBlank();
  std::string script = R"(
    function writeIframe() {
      let frameDocument = document.getElementById("about_blank").contentWindow.document;
      frameDocument.open("text/html");
      frameDocument.write(`
        <html>
          <body>
            <iframe id='middle_frame' src='/cross-site/middle.com/middle_frame.html' width='400'></iframe>
          </body>
        </html>`);
      frameDocument.close();
    }
    if (document.readyState == "complete") {
      writeIframe();
    } else {
      document.getElementById("about_blank").addEventListener("load", writeIframe);
    }
  )";
  EXPECT_TRUE(content::ExecJs(
      browser()->tab_strip_model()->GetActiveWebContents(), script));
  VerifyTargetResourceBlockingStatus(false);
  observer.VerifyNotificationSent("resource.png",
                                  observer.blocked_ads_notifications);
  SetFilters({"@@^eyeo=true$document"});
  NavigateToOutermostFrameWithAboutBlank();
  EXPECT_TRUE(content::ExecJs(
      browser()->tab_strip_model()->GetActiveWebContents(), script));
  VerifyTargetResourceBlockingStatus(true);
  observer.VerifyNotificationSent("resource.png",
                                  observer.allowed_ads_notifications);
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockFrameHierarchyBrowserTest,
                         testing::Values(false /*normal*/, true /*incognito*/));

// More tests can be added / parametrized, e.g.:
// - elemhide blocking filters (in conjunction with $elemhide allow rules)
// - $subdocument-based allow rules

}  // namespace adblock
