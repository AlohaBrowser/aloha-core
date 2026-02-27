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

#ifndef ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_WEBCONTENTS_OBSERVER_H_
#define ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_WEBCONTENTS_OBSERVER_H_

#include "components/adblock/content/browser/adblock_webcontents_observer_base.h"
#include "content/public/browser/web_contents_user_data.h"

namespace android_webview {

/**
 * @brief Listens to page load events to trigger frame-wide element hiding.
 * Responds to notifications about blocked resource loads to collapse the
 * empty space around them. Lives in browser process UI thread.
 *
 */
class AdblockAwWebContentObserver
    : public adblock::AdblockWebContentObserverBase,
      public content::WebContentsUserData<AdblockAwWebContentObserver> {
 public:
  AdblockAwWebContentObserver(
      content::WebContents* web_contents,
      adblock::SubscriptionService* subscription_service,
      adblock::ElementHider* element_hider,
      adblock::SitekeyStorage* sitekey_storage,
      std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder,
      base::RepeatingCallback<void(content::RenderFrameHost*)>
          navigation_counter);
  ~AdblockAwWebContentObserver() override;

 protected:
  bool IsAdblockEnabled(content::RenderFrameHost* frame) const override;

 private:
  friend class content::WebContentsUserData<AdblockAwWebContentObserver>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_WEBCONTENTS_OBSERVER_H_
