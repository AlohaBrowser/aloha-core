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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_ADBLOCK_BROWSERTEST_BASE_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_ADBLOCK_BROWSERTEST_BASE_H_

#include "base/run_loop.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/app/shell_main_delegate.h"

namespace adblock {

class TestResourceClassificationRunnerObserver
    : public ResourceClassificationRunner::Observer {
 public:
  TestResourceClassificationRunnerObserver();

  ~TestResourceClassificationRunnerObserver() override;

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override;

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override;
  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override;

  std::vector<GURL> blocked_ads_notifications;
  std::vector<GURL> allowed_ads_notifications;
  std::vector<GURL> blocked_popups_notifications;
  std::vector<GURL> allowed_popups_notifications;
  std::vector<GURL> allowed_pages_notifications;
};

class SubscriptionInstalledWaiter
    : public SubscriptionService::SubscriptionObserver {
 public:
  explicit SubscriptionInstalledWaiter(
      SubscriptionService* subscription_service);

  ~SubscriptionInstalledWaiter() override;

  void WaitUntilSubscriptionsInstalled(std::vector<GURL> subscriptions);

  void OnSubscriptionInstalled(const GURL& subscription_url) override;

 protected:
  raw_ptr<SubscriptionService> subscription_service_;
  base::RunLoop run_loop_;
  std::vector<GURL> awaited_subscriptions_;
};

class AdblockBrowserTestBase : public content::ContentBrowserTest {
 public:
  AdblockBrowserTestBase();

  ~AdblockBrowserTestBase() override;

  // Without this override there is no AdblockShellContentBrowserClient
  // (created by ShellMainDelegate) but default ShellContentBrowserClient.
  content::ContentMainDelegate* GetOptionalContentMainDelegateOverride()
      override;

  content::WebContents* web_contents();

  content::BrowserContext* browser_context();

  PrefService* GetPrefs();

  SubscriptionInstalledWaiter GetSubscriptionInstalledWaiter();

  void SetUpOnMainThread() override;

  void TearDownOnMainThread() override;

  bool WaitAndVerifyCondition(const char* condition);

  void NotifyTestFinished();

  void RunUntilTestFinished();

  // Sets custom filters in "adblock" configuration
  void SetFilters(const std::vector<std::string>& filters);

  void InitResourceClassificationObserver();

  std::string GetTelemetryDomain();

 protected:
  bool finish_condition_met_ = false;
  base::RepeatingClosure quit_closure_;
  TestResourceClassificationRunnerObserver observer_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_ADBLOCK_BROWSERTEST_BASE_H_
