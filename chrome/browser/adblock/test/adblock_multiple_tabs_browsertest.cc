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
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/recent_tabs_sub_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
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
        browser()->GetProfile()->GetOriginalProfile())
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
            browser()->GetProfile()->GetOriginalProfile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    adblock_configuration->RemoveCustomFilter(kAllowlistEverythingFilter);
    for (auto& filter : filters) {
      adblock_configuration->AddCustomFilter(filter);
    }
  }

  void RestoreTabs(BrowserWindowInterface* browser) {
    RecentTabsSubMenuModel menu(nullptr, browser);
    menu.ExecuteCommand(menu.GetFirstRecentTabsCommandId(), 0);

    // Find the restored browser (may be a new browser or the current one)
    BrowserWindowInterface* restored_browser = nullptr;
    const size_t browser_count =
        GlobalBrowserCollection::GetInstance()->GetSize();
    if (browser_count > 1) {
      // A new browser was created
      auto browsers = GetAllBrowserWindowInterfaces();
      restored_browser = browsers.back();
    } else {
      // Tabs restored to current browser
      restored_browser = browser;
    }

    // Wait on the restored tab strip rather than on DOM messages from every
    // WebContents created during restore: unrelated WebContents (side panels,
    // preloads) never send "READY" and would block until the test times out.
    // Restored tabs may also start out unloaded, hence LoadIfNecessary().
    TabStripModel* tab_strip_model = restored_browser->tab_strip_model();
    int restored_tabs = 0;
    for (int i = 0; i < tab_strip_model->count(); ++i) {
      content::WebContents* contents = tab_strip_model->GetWebContentsAt(i);
      if (contents->GetVisibleURL().path() != kTestPagePath) {
        continue;
      }
      contents->GetController().LoadIfNecessary();
      ASSERT_TRUE(content::WaitForLoadStop(contents));
      ++restored_tabs;
    }
    ASSERT_EQ(kTabsCount, restored_tabs);
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
  const char* kTestPagePath = "/tab-restore.html";
  std::set<int> tabs_with_blocked_resource_;
  std::set<int> tabs_with_allowed_resource_;
};

IN_PROC_BROWSER_TEST_F(AdblockMultipleTabsBrowserTest, PRE_OpenManyTabs) {
  // Load page in already opened tab
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kTestDomain, kTestPagePath)));
  // Open more tabs
  for (int i = 0; i < kTabsCount - 1; ++i) {
    ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(), embedded_test_server()->GetURL(kTestDomain, kTestPagePath),
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
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  // Close the 1st browser and clear tabs test data
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
  tabs_with_blocked_resource_.clear();
  tabs_with_allowed_resource_.clear();

  auto browsers = GetAllBrowserWindowInterfaces();
  BrowserWindowInterface* browser = browsers.front();
  // Restore tabs from 1st browser instance (already closed) in 2nd instance
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

  GURL target_page = embedded_test_server()->GetURL(kTestDomain, kTestPagePath);

  // Open kTabsCount tabs manually
  for (int i = 0; i < kTabsCount; ++i) {
    if (i == 0) {
      // First tab: navigate in current tab
      ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target_page));
    } else {
      // Subsequent tabs: open in new tabs
      ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
          browser(), target_page, WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
    }
  }

  // Verify that ad-blocking worked for all tabs
  EXPECT_EQ(kTabsCount, browser()->tab_strip_model()->count());
  EXPECT_EQ(kTabsCount, static_cast<int>(tabs_with_blocked_resource_.size()));
  EXPECT_EQ(kTabsCount, static_cast<int>(tabs_with_allowed_resource_.size()));
}

}  // namespace adblock
