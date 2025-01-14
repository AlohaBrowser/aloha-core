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

#include "components/adblock/content/browser/test/adblock_browsertest_base.h"

#include "components/adblock/content/browser/factories/adblock_request_throttle_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/activeping_telemetry_topic_provider.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/net/adblock_request_throttle.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace adblock {

TestResourceClassificationRunnerObserver::
    TestResourceClassificationRunnerObserver() = default;

TestResourceClassificationRunnerObserver::
    ~TestResourceClassificationRunnerObserver() = default;

void TestResourceClassificationRunnerObserver::OnRequestMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  if (match_result == FilterMatchResult::kAllowRule) {
    allowed_ads_notifications.push_back(url);
  } else {
    blocked_ads_notifications.push_back(url);
  }
}

void TestResourceClassificationRunnerObserver::OnPageAllowed(
    const GURL& url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  allowed_pages_notifications.push_back(url);
}

void TestResourceClassificationRunnerObserver::OnPopupMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const GURL& opener_url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  if (match_result == FilterMatchResult::kAllowRule) {
    allowed_popups_notifications.push_back(url);
  } else {
    blocked_popups_notifications.push_back(url);
  }
}

SubscriptionInstalledWaiter::SubscriptionInstalledWaiter(
    SubscriptionService* subscription_service)
    : subscription_service_(subscription_service) {
  subscription_service_->AddObserver(this);
}

SubscriptionInstalledWaiter::~SubscriptionInstalledWaiter() {
  subscription_service_->RemoveObserver(this);
}

void SubscriptionInstalledWaiter::WaitUntilSubscriptionsInstalled(
    std::vector<GURL> subscriptions) {
  awaited_subscriptions_ = std::move(subscriptions);
  run_loop_.Run();
}

void SubscriptionInstalledWaiter::OnSubscriptionInstalled(
    const GURL& subscription_url) {
  awaited_subscriptions_.erase(
      base::ranges::remove(awaited_subscriptions_, subscription_url),
      awaited_subscriptions_.end());
  if (awaited_subscriptions_.empty()) {
    run_loop_.Quit();
  }
}

AdblockBrowserTestBase::AdblockBrowserTestBase() = default;

AdblockBrowserTestBase::~AdblockBrowserTestBase() = default;

content::ContentMainDelegate*
AdblockBrowserTestBase::GetOptionalContentMainDelegateOverride() {
  return new content::ShellMainDelegate(true);
}

content::WebContents* AdblockBrowserTestBase::web_contents() {
  return shell()->web_contents();
}

content::BrowserContext* AdblockBrowserTestBase::browser_context() {
  return web_contents()->GetBrowserContext();
}

PrefService* AdblockBrowserTestBase::GetPrefs() {
  return user_prefs::UserPrefs::Get(browser_context());
}

SubscriptionInstalledWaiter
AdblockBrowserTestBase::GetSubscriptionInstalledWaiter() {
  return SubscriptionInstalledWaiter(
      SubscriptionServiceFactory::GetForBrowserContext(browser_context()));
}

void AdblockBrowserTestBase::SetUpOnMainThread() {
  content::ContentBrowserTest::SetUpOnMainThread();
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  // Some tests remove "adblock" configuration so let's check before using.
  if (adblock_configuration) {
    adblock_configuration->RemoveCustomFilter(kAllowlistEverythingFilter);
  }

  // Allow network requests immediately, otherwise tests that expect e.g. filter
  // list downloads will hang for 30 seconds.
  AdblockRequestThrottleFactory::GetForBrowserContext(browser_context())
      ->AllowRequestsAfter(base::Seconds(0));
}

void AdblockBrowserTestBase::TearDownOnMainThread() {
  // RemoveObserver is harmless even if AddObserver was not called.
  auto* classification_runner =
      ResourceClassificationRunnerFactory::GetForBrowserContext(
          browser_context());
  classification_runner->RemoveObserver(&observer_);
}

bool AdblockBrowserTestBase::WaitAndVerifyCondition(const char* condition) {
  std::string script = base::StringPrintf(
      R"(
      (async () => {
        let count = 10;
        function waitFor(condition) {
          const poll = resolve => {
            if(condition() || !count--) resolve();
            else setTimeout(_ => poll(resolve), 300);
          }
          return new Promise(poll);
        }
        // Waits up to 3 seconds
        await waitFor(_ => %s);
        return %s;
     })()
     )",
      condition, condition);
  return content::EvalJs(web_contents(), script) == true;
}

void AdblockBrowserTestBase::NotifyTestFinished() {
  finish_condition_met_ = true;
  // If the test is currently waiting for the finish condition to be met, we
  // need to quit the run loop.
  if (quit_closure_) {
    quit_closure_.Run();
  }
}

void AdblockBrowserTestBase::RunUntilTestFinished() {
  // If the finish condition is already met, we don't need to run the run
  // loop.
  if (finish_condition_met_) {
    return;
  }
  // Wait until NotifyTestFinished() gets called.
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  std::move(run_loop).Run();
}

void AdblockBrowserTestBase::SetFilters(
    const std::vector<std::string>& filters) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration) << "Test expects \"adblock\" configuration";
  for (auto& filter : filters) {
    adblock_configuration->AddCustomFilter(filter);
  }
}

void AdblockBrowserTestBase::InitResourceClassificationObserver() {
  auto* classification_runner =
      ResourceClassificationRunnerFactory::GetForBrowserContext(
          browser_context());
  DCHECK(classification_runner);
  classification_runner->AddObserver(&observer_);
}

std::string AdblockBrowserTestBase::GetTelemetryDomain() {
  static std::string domain =
      GURL(ActivepingTelemetryTopicProvider::DefaultBaseUrl()).host();
  return domain;
}

}  // namespace adblock
