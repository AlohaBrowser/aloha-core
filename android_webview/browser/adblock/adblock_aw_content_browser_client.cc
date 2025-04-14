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

#include "android_webview/browser/adblock/adblock_aw_content_browser_client.h"

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_contents.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"

namespace android_webview {

AdblockAwContentBrowserClient::AdblockAwContentBrowserClient(
    AwFeatureListCreator* aw_feature_list_creator)
    : adblock::AdblockContentBrowserClient<AwContentBrowserClient>(
          aw_feature_list_creator) {}

bool AdblockAwContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const url::Origin& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(opener);

  auto* browser_context =
      GetBrowserContextForEyeoFactories(opener->GetBrowserContext());

  if (IsFilteringNeeded(opener, browser_context)) {
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(opener);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            browser_context);
    GURL popup_url(target_url);
    web_contents->GetPrimaryMainFrame()->GetProcess()->FilterURL(false,
                                                                 &popup_url);
    auto* classification_runner =
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser_context);
    const auto popup_blocking_decision =
        classification_runner->ShouldBlockPopup(
            subscription_service->GetCurrentSnapshot(), popup_url, opener);
    if (popup_blocking_decision == adblock::FilterMatchResult::kAllowRule) {
      return true;
    }
    if (popup_blocking_decision == adblock::FilterMatchResult::kBlockRule) {
      return false;
    }
    // Otherwise, if eyeo adblocking is disabled or there is no rule that
    // explicitly allows or blocks a popup, fall back on Chromium's built-in
    // popup blocker.
    DCHECK(popup_blocking_decision == adblock::FilterMatchResult::kDisabled ||
           popup_blocking_decision == adblock::FilterMatchResult::kNoRule);
  }

  return AwContentBrowserClient::CanCreateWindow(
      opener, opener_url, opener_top_level_frame_url, source_origin,
      container_type, target_url, referrer, frame_name, disposition, features,
      user_gesture, opener_suppressed, no_javascript_access);
}

bool AdblockAwContentBrowserClient::IsFilteringNeeded(
    content::RenderFrameHost* frame,
    content::BrowserContext* browser_context) {
  if (frame) {
    auto* wc = content::WebContents::FromRenderFrameHost(frame);
    auto* aw = AwContents::FromWebContents(wc);
    if (aw && !aw->IsContentFilteringEnabled()) {
      return false;
    }
  }
  return adblock::AdblockContentBrowserClient<
      AwContentBrowserClient>::IsFilteringNeeded(frame, browser_context);
}

content::BrowserContext*
AdblockAwContentBrowserClient::GetBrowserContextForEyeoFactories(
    content::BrowserContext* /*current_browser_context*/) {
  return android_webview::AwBrowserContext::GetDefault();
}

}  // namespace android_webview
