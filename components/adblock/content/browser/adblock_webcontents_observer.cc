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

#include "components/adblock/content/browser/adblock_webcontents_observer.h"

#include "base/trace_event/trace_event.h"
#include "components/adblock/content/browser/frame_opener_info.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/frame/frame_owner_element_type.h"

namespace {
const char* WindowOpenDispositionToString(WindowOpenDisposition value) {
  switch (value) {
    case WindowOpenDisposition::UNKNOWN:
      return "UNKNOWN";
    case WindowOpenDisposition::CURRENT_TAB:
      return "CURRENT_TAB";
    case WindowOpenDisposition::SINGLETON_TAB:
      return "SINGLETON_TAB";
    case WindowOpenDisposition::NEW_FOREGROUND_TAB:
      return "NEW_FOREGROUND_TAB";
    case WindowOpenDisposition::NEW_BACKGROUND_TAB:
      return "NEW_BACKGROUND_TAB";
    case WindowOpenDisposition::NEW_POPUP:
      return "NEW_POPUP";
    case WindowOpenDisposition::NEW_WINDOW:
      return "NEW_WINDOW";
    case WindowOpenDisposition::SAVE_TO_DISK:
      return "SAVE_TO_DISK";
    case WindowOpenDisposition::OFF_THE_RECORD:
      return "OFF_THE_RECORD";
    case WindowOpenDisposition::IGNORE_ACTION:
      return "IGNORE_ACTION";
    case WindowOpenDisposition::SWITCH_TO_TAB:
      return "SWITCH_TO_TAB";
    case WindowOpenDisposition::NEW_PICTURE_IN_PICTURE:
      return "NEW_PICTURE_IN_PICTURE";
    default:
      return "";
  }
}
}  // namespace

void TraceHandleLoadComplete(
    intptr_t rfh_trace_id,
    const adblock::ElementHider::ElemhideInjectionData&) {
  TRACE_EVENT_NESTABLE_ASYNC_END0("eyeo",
                                  "AdblockWebContentObserver::HandleOnLoad",
                                  TRACE_ID_LOCAL(rfh_trace_id));
}

AdblockWebContentObserver::AdblockWebContentObserver(
    content::WebContents* web_contents,
    adblock::SubscriptionService* subscription_service,
    adblock::ElementHider* element_hider,
    adblock::SitekeyStorage* sitekey_storage,
    std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdblockWebContentObserver>(*web_contents),
      subscription_service_(subscription_service),
      element_hider_(element_hider),
      sitekey_storage_(sitekey_storage),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)) {}

AdblockWebContentObserver::~AdblockWebContentObserver() = default;

void AdblockWebContentObserver::DidOpenRequestedURL(
    content::WebContents* new_contents,
    content::RenderFrameHost* source_render_frame_host,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    ui::PageTransition transition,
    bool started_from_context_menu,
    bool renderer_initiated) {
  if (!IsAdblockEnabled()) {
    return;
  }
  VLOG(1) << "[eyeo] DidOpenRequestedURL: URL=" << url
          << ", disposition=" << WindowOpenDispositionToString(disposition)
          << ", renderer_initiated=" << renderer_initiated;
  if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
      disposition == WindowOpenDisposition::NEW_POPUP) {
    // WindowOpenDisposition::NEW_WINDOW is excluded as it is set when user
    // opens a link in a new window from context menu.
    // We use content::WebContentsUserData instead of content::DocumentUserData
    // because the latter is reset when there is a client side redirection.
    adblock::FrameOpenerInfo::CreateForWebContents(new_contents);
    auto* info = adblock::FrameOpenerInfo::FromWebContents(new_contents);
    info->SetOpener(source_render_frame_host->GetGlobalId());
  }
}

void AdblockWebContentObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsAdblockEnabled()) {
    return;
  }
  const GURL& url = navigation_handle->GetURL();
  VLOG(1) << "[eyeo] Finished navigation: URL=" << url
          << ", has_commited=" << navigation_handle->HasCommitted()
          << ", is_error=" << navigation_handle->IsErrorPage()
          << ", isInMainFrame=" << navigation_handle->IsInMainFrame();
  if (navigation_handle->HasCommitted()) {
    if (!navigation_handle->GetRenderFrameHost()) {
      return;
    }
    if (!navigation_handle->IsErrorPage()) {
      DVLOG(3) << "[eyeo] Ready to inject JS to " << url.spec();
      HandleOnLoad(navigation_handle->GetRenderFrameHost());
    }
    if (navigation_handle->IsErrorPage() || url.IsAboutBlank() ||
        !url.SchemeIsHTTPOrHTTPS()) {
      DVLOG(3) << "[eyeo] Not suitable URL " << url.spec()
               << ", error = " << navigation_handle->GetNetErrorCode()
               << ", IsAboutSrcdoc = " << url.IsAboutSrcdoc();

      if (navigation_handle->GetNetErrorCode() ==
              net::ERR_BLOCKED_BY_ADMINISTRATOR &&
          navigation_handle->GetRenderFrameHost()->GetFrameOwnerElementType() ==
              blink::FrameOwnerElementType::kIframe) {
        // element hiding for blocked element
        DVLOG(3) << "[eyeo] Subframe url=" << url.spec()
                 << ", isAboutSrcDoc = " << url.IsAboutSrcdoc();
        element_hider_->HideBlockedElement(
            url, navigation_handle->GetRenderFrameHost()->GetParent());
      }
    }
  }
}

void AdblockWebContentObserver::HandleOnLoad(
    content::RenderFrameHost* frame_host) {
  DCHECK(frame_host);
  const GURL url =
      frame_hierarchy_builder_->FindUrlForFrame(frame_host, web_contents());
  const auto rfh_trace_id = reinterpret_cast<intptr_t>(frame_host);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      "eyeo", "AdblockWebContentObserver::HandleOnLoad",
      TRACE_ID_LOCAL(rfh_trace_id), "url", url.spec());

  auto frame_hierarchy = frame_hierarchy_builder_->BuildFrameHierarchy(
      adblock::RequestInitiator(frame_host));
  DVLOG(1) << "[eyeo] Got " << frame_hierarchy.size()
           << " frame hierarchy items for " << url.spec();

  adblock::SiteKey site_key;
  auto url_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(frame_hierarchy);
  if (url_key_pair.has_value()) {
    site_key = url_key_pair->second;
    DVLOG(2) << "[eyeo] Element hiding found siteKey: " << url_key_pair->second
             << " for url: " << url_key_pair->first;
  }

  element_hider_->ApplyElementHidingEmulationOnPage(
      std::move(url), std::move(frame_hierarchy), frame_host,
      std::move(site_key),
      base::BindOnce(&TraceHandleLoadComplete, rfh_trace_id));
}

bool AdblockWebContentObserver::IsAdblockEnabled() const {
  return base::ranges::any_of(
      subscription_service_->GetInstalledFilteringConfigurations(),
      &adblock::FilteringConfiguration::IsEnabled) 
          && web_contents() && web_contents()->IsAdblockEnabled(); // ALOHA https://app.clickup.com/t/86epcdndk
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdblockWebContentObserver);
