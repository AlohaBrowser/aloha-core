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

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/recent_tabs_sub_menu_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockMultipleTabsBrowserTest
    : public InProcessBrowserTest,
      public ResourceClassificationRunner::Observer {
 public:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule(kTestDomain, "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
    ResourceClassificationRunnerFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->AddObserver(this);
    SetFilters({"blocked.png", "allowed.png", "@@allowed.png"});
  }

  void TearDownInProcessBrowserTestFixture() override {
    ASSERT_EQ(kTabsCount, static_cast<int>(tabs_with_blocked_resource_.size()));
    ASSERT_EQ(kTabsCount, static_cast<int>(tabs_with_allowed_resource_.size()));
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

  void RestoreTabs(Browser* browser) {
    content::DOMMessageQueue queue;
    RecentTabsSubMenuModel menu(nullptr, browser);
    menu.ExecuteCommand(menu.GetFirstRecentTabsCommandId(), 0);
    for (int i = 0; i < kTabsCount; ++i) {
      std::string message;
      EXPECT_TRUE(queue.WaitForMessage(&message));
      EXPECT_EQ("\"READY\"", message);
    }
  }

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {
    const content::WebContents* wc =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    if (match_result == FilterMatchResult::kBlockRule &&
        url.path() == "/blocked.png") {
      tabs_with_blocked_resource_.insert(
          sessions::SessionTabHelper::IdForTab(wc).id());
    } else if (match_result == FilterMatchResult::kAllowRule &&
               url.path() == "/allowed.png") {
      tabs_with_allowed_resource_.insert(
          sessions::SessionTabHelper::IdForTab(wc).id());
    }
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {}

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {}

 protected:
  const int kTabsCount = 4;
  const char* kTestDomain = "example.com";
  std::set<int> tabs_with_blocked_resource_;
  std::set<int> tabs_with_allowed_resource_;
};

IN_PROC_BROWSER_TEST_F(AdblockMultipleTabsBrowserTest, PRE_OpenManyTabs) {
  // Load page in already opened tab
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL(kTestDomain, "/tab-restore.html")));
  // Open more tabs
  for (int i = 0; i < kTabsCount - 1; ++i) {
    ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(),
        embedded_test_server()->GetURL(kTestDomain, "/tab-restore.html"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  }
  EXPECT_EQ(kTabsCount, browser()->tab_strip_model()->count());
  EXPECT_EQ(kTabsCount, static_cast<int>(tabs_with_blocked_resource_.size()));
  EXPECT_EQ(kTabsCount, static_cast<int>(tabs_with_allowed_resource_.size()));

  // Open a new browser instance
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(url::kAboutBlankURL), WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  BrowserList* active_browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2u, active_browser_list->size());

  // Close the 1st browser and clear tabs test data
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(1u, active_browser_list->size());
  tabs_with_blocked_resource_.clear();
  tabs_with_allowed_resource_.clear();

  Browser* browser = active_browser_list->get(0);
  // Restore tabs from1 st browser instance (already closed) in 2nd instance
  RestoreTabs(browser);
}

// TODO(atokodi): Enable this test once it works on OSX. It currently does not
// work because of an upstream bug. See DPD-2528
#if BUILDFLAG(IS_MAC)
#define MAYBE_OpenManyTabs DISABLED_OpenManyTabs
#else
#define MAYBE_OpenManyTabs OpenManyTabs
#endif

IN_PROC_BROWSER_TEST_F(AdblockMultipleTabsBrowserTest, MAYBE_OpenManyTabs) {
  ASSERT_EQ(0u, tabs_with_blocked_resource_.size());
  ASSERT_EQ(0u, tabs_with_allowed_resource_.size());
  // Restore tabs from previous session (previous test)
  RestoreTabs(browser());
}

}  // namespace adblock
