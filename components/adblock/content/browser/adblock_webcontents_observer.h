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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/core/sitekey_storage.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}  // namespace content

class GURL;

namespace adblock {

/**
 * @brief Listens to page load events to trigger frame-wide element hiding.
 * Responds to notifications about blocked resource loads to collapse the
 * empty space around them. Lives in browser process UI thread.
 *
 */
class AdblockWebContentObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<AdblockWebContentObserver> {
 public:
  AdblockWebContentObserver(
      content::WebContents* web_contents,
      SubscriptionService* subscription_service,
      ElementHider* element_hider,
      SitekeyStorage* sitekey_storage,
      std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder,
      base::RepeatingCallback<void(content::RenderFrameHost*)>
          navigation_counter);
  ~AdblockWebContentObserver() override;
  AdblockWebContentObserver(const AdblockWebContentObserver&) = delete;
  AdblockWebContentObserver& operator=(const AdblockWebContentObserver&) =
      delete;

  // WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void DidOpenRequestedURL(content::WebContents* new_contents,
                           content::RenderFrameHost* source_render_frame_host,
                           const GURL& url,
                           const content::Referrer& referrer,
                           WindowOpenDisposition disposition,
                           ui::PageTransition transition,
                           bool started_from_context_menu,
                           bool renderer_initiated) override;

 private:
  explicit AdblockWebContentObserver(content::WebContents* web_contents);
  void HandleOnLoad(content::RenderFrameHost* render_frame_host);
  bool IsAdblockEnabled() const;

  friend class content::WebContentsUserData<AdblockWebContentObserver>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  raw_ptr<SubscriptionService> subscription_service_;
  raw_ptr<ElementHider> element_hider_;
  raw_ptr<SitekeyStorage> sitekey_storage_;

  std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder_;
  base::RepeatingCallback<void(content::RenderFrameHost*)> navigation_counter_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_
