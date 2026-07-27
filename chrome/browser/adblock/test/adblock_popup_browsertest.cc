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

#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_browsertest_util.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "gmock/gmock.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

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

enum class Redirection { ClientSide, ServerSide };

}  // namespace

class AdblockPopupBrowserTest
    : public InProcessBrowserTest,
      public ResourceClassificationRunner::Observer,
      public testing::WithParamInterface<Redirection> {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &AdblockPopupBrowserTest::RequestHandler, base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
    ResourceClassificationRunnerFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->AddObserver(this);
  }

  void TearDownOnMainThread() override {
    VerifyNoUnexpectedNotifications();
    ResourceClassificationRunnerFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->RemoveObserver(this);
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void VerifyNoUnexpectedNotifications() {
    EXPECT_TRUE(blocked_popups_notifications_expectations_.empty());
    EXPECT_TRUE(allowed_popups_notifications_expectations_.empty());
  }

  bool IsServerSideRedirection() {
    return GetParam() == Redirection::ServerSide;
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (base::StartsWith("/main_page.html", request.relative_url)) {
      static constexpr char kPopupFrameParent[] =
          R"(
        <!DOCTYPE html>
        <html>
          <head></head>
          <body>
            <iframe id='popup_frame' src='/popup_frame.html'></iframe>
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kPopupFrameParent);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }
    if (base::StartsWith("/popup_frame.html", request.relative_url)) {
      static constexpr char kPopupFrame[] =
          R"(
        <!DOCTYPE html>
        <html>
          <head>
            <script>
              "use strict";
              let redirect_popup_url = "/popup_will_redirect.html";
              function scriptPopupTab() {
                window.open(redirect_popup_url);
              }
              function scriptPopupWindow() {
                window.open(redirect_popup_url, "some-popup", "resizable");
              }
            </script>
          </head>
          <body>
            <a href="/popup_will_redirect.html" target="_blank" id="popup_link_will_redirect">Trigger link based popup with redirect</a>
            <a href="/popup_no_redirect.html" target="_blank" id="popup_link_no_redirect">Trigger link based popup without redirect</a>
            <a href="#script-based-popup-tab" onclick="scriptPopupTab();" id="popup_script_tab">Trigger script based popup (tab) with redirect</a>
            <a href="#script-based-popup-window" onclick="scriptPopupWindow();" id="popup_script_window">Trigger script based popup (window) with redirect</a>
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kPopupFrame);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }
    if (base::StartsWith("/popup_no_redirect.html", request.relative_url) ||
        base::StartsWith("/popup_redirected.html", request.relative_url)) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content("");
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }
    if (base::StartsWith("/popup_will_redirect.html", request.relative_url)) {
      static constexpr char kClientSideRedirectingPopup[] =
          R"(
        <!DOCTYPE html>
        <html>
          <head>
            <script type="text/javascript">
              window.location = "/popup_redirected.html"
            </script>
          </head>
          <body></body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      if (IsServerSideRedirection()) {
        http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
        http_response->AddCustomHeader(
            "Location", GetPageUrl("/popup_redirected.html").spec());
      } else {
        http_response->set_code(net::HTTP_OK);
        http_response->set_content(kClientSideRedirectingPopup);
        http_response->set_content_type("text/html");
      }
      return std::move(http_response);
    }
    return nullptr;
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

  void TriggerPopup(const std::string& popup_id) {
    std::string script = base::StringPrintf(
        R"(
      let doc = document.querySelector('iframe[id="popup_frame"]').contentWindow.document;
      let element = doc.getElementById('%s');
      element.click();
    )",
        popup_id.c_str());
    EXPECT_TRUE(content::ExecJs(
        browser()->tab_strip_model()->GetActiveWebContents(), script));
  }

  GURL GetPageUrl(const std::string& page) {
    return embedded_test_server()->GetURL("popup_frame.org", page);
  }

  void NavigateToPage() {
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GetPageUrl("/main_page.html")));
  }

  void WaitForTabToLoad() {
    content::WebContents* popup =
        browser()->tab_strip_model()->GetActiveWebContents();
    WaitForLoadStop(popup);
  }

  void SetupNotificationsWaiter(base::RunLoop* run_loop) {
    run_loop_ = run_loop;
  }

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {}

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {}

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {
    auto& list = (match_result == FilterMatchResult::kBlockRule
                      ? blocked_popups_notifications_expectations_
                      : allowed_popups_notifications_expectations_);
    auto it = std::find(list.begin(), list.end(), url.ExtractFileName());
    ASSERT_FALSE(it == list.end())
        << "Path " << url.ExtractFileName() << " not on list";
    list.erase(it);
    if (run_loop_ && allowed_popups_notifications_expectations_.empty() &&
        blocked_popups_notifications_expectations_.empty()) {
      run_loop_->Quit();
    }
  }

  std::vector<std::string> allowed_popups_notifications_expectations_;
  std::vector<std::string> blocked_popups_notifications_expectations_;
  raw_ptr<base::RunLoop> run_loop_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(AdblockPopupBrowserTest, PopupLinkBlocked) {
  SetFilters({"popup_no_redirect.html^$popup"});
  blocked_popups_notifications_expectations_.emplace_back(
      "popup_no_redirect.html");
  NavigateToPage();
  TabAddedRemovedObserver observer(browser()->tab_strip_model());
  TriggerPopup("popup_link_no_redirect");
  observer.Wait();
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       PopupScriptTabWithRedirectBlocked) {
  SetFilters({"popup_redirected.html^$popup"});
  blocked_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  TabAddedRemovedObserver observer(browser()->tab_strip_model());
  TriggerPopup("popup_script_tab");
  observer.Wait();
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       PopupScriptWindowWithRedirectBlocked) {
  SetFilters({"popup_redirected.html^$popup"});
  blocked_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  TriggerPopup("popup_script_window");
  // Wait for 2nd browser to get closed (new window popup blocked)
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());
  auto browsers = GetAllBrowserWindowInterfaces();
  ui_test_utils::WaitForBrowserToClose(
      browsers[1]->GetBrowserForMigrationOnly());
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest, PopupLinkWithRedirectBlocked) {
  SetFilters({"popup_redirected.html^$popup"});
  blocked_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  TabAddedRemovedObserver observer(browser()->tab_strip_model());
  TriggerPopup("popup_link_will_redirect");
  observer.Wait();
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_P(
    AdblockPopupBrowserTest,
    PopupScriptTabWithRedirectAllowedByIntermediateParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/popup_frame.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  ui_test_utils::TabAddedWaiter waiter(browser());
  TriggerPopup("popup_script_tab");
  waiter.Wait();
  WaitForTabToLoad();
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_P(
    AdblockPopupBrowserTest,
    PopupScriptWindowWithRedirectAllowedByIntermediateParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/popup_frame.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  base::RunLoop run_loop;
  SetupNotificationsWaiter(&run_loop);
  TriggerPopup("popup_script_window");
  run_loop.Run();
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());
}

IN_PROC_BROWSER_TEST_P(
    AdblockPopupBrowserTest,
    PopupLinkWithRedirectAllowedByIntermediateParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/popup_frame.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  ui_test_utils::TabAddedWaiter waiter(browser());
  TriggerPopup("popup_link_will_redirect");
  waiter.Wait();
  WaitForTabToLoad();
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
  ;
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       PopupScriptTabWithRedirectAllowedByParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/main_page.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  ui_test_utils::TabAddedWaiter waiter(browser());
  TriggerPopup("popup_script_tab");
  waiter.Wait();
  WaitForTabToLoad();
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       PopupScriptWindowWithRedirectAllowedByParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/main_page.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  base::RunLoop run_loop;
  SetupNotificationsWaiter(&run_loop);
  TriggerPopup("popup_script_window");
  run_loop.Run();
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());
}

IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       PopupLinkWithRedirectAllowedByParentDocument) {
  SetFilters({"popup_redirected.html^$popup", "@@/main_page.html^$document"});
  allowed_popups_notifications_expectations_.emplace_back(
      "popup_redirected.html");
  NavigateToPage();
  ui_test_utils::TabAddedWaiter waiter(browser());
  TriggerPopup("popup_link_will_redirect");
  waiter.Wait();
  WaitForTabToLoad();
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
}

// Make sure that we correctly recognize and apply blocking of
// redirected popups only for real popups.
IN_PROC_BROWSER_TEST_P(AdblockPopupBrowserTest,
                       LinkOpenedByContextMenuInNewTabNotBlocked) {
  SetFilters({"popup_redirected.html^$popup"});
  ContextMenuNotificationObserver menu_observer(
      IDC_CONTENT_CONTEXT_OPENLINKNEWTAB);
  ui_test_utils::AllBrowserTabAddedWaiter add_tab;

  std::string script = base::StringPrintf(
      "data:text/html,<a href='%s'>link</a>",
      GetPageUrl("/popup_will_redirect.html").spec().c_str());
  // Go to a page with a link
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(script)));

  // Opens a link in a new tab via a "real" context menu.
  blink::WebMouseEvent mouse_event(
      blink::WebInputEvent::Type::kMouseDown,
      blink::WebInputEvent::kNoModifiers,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  mouse_event.button = blink::WebMouseEvent::Button::kRight;
  mouse_event.SetPositionInWidget(15, 15);
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  gfx::Rect offset = tab->GetContainerBounds();
  mouse_event.SetPositionInScreen(15 + offset.x(), 15 + offset.y());
  mouse_event.click_count = 1;
  tab->GetPrimaryMainFrame()
      ->GetRenderViewHost()
      ->GetWidget()
      ->ForwardMouseEvent(mouse_event);
  mouse_event.SetType(blink::WebInputEvent::Type::kMouseUp);
  tab->GetPrimaryMainFrame()
      ->GetRenderViewHost()
      ->GetWidget()
      ->ForwardMouseEvent(mouse_event);

  // The menu_observer will select "Open in new tab", wait for the new tab to
  // be added.
  tab = add_tab.Wait();
  EXPECT_TRUE(content::WaitForLoadStop(tab));

  // Verify that it's the correct tab.
  EXPECT_EQ(GetPageUrl("/popup_redirected.html"), tab->GetLastCommittedURL());
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockPopupBrowserTest,
                         testing::Values(Redirection::ClientSide,
                                         Redirection::ServerSide));

}  // namespace adblock
