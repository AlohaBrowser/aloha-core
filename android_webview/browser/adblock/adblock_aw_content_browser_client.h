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

#ifndef ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_CONTENT_BROWSER_CLIENT_H_
#define ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_CONTENT_BROWSER_CLIENT_H_

#include "components/adblock/content/browser/adblock_content_browser_client.h"

#include "android_webview/browser/aw_content_browser_client.h"

namespace android_webview {

class AdblockAwContentBrowserClient
    : public adblock::AdblockContentBrowserClient<AwContentBrowserClient> {
 public:
  // |aw_feature_list_creator| should not be null.
  explicit AdblockAwContentBrowserClient(
      AwFeatureListCreator* aw_feature_list_creator);

  bool CanCreateWindow(content::RenderFrameHost* opener,
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
                       bool* no_javascript_access) override;

 private:
  content::BrowserContext* GetBrowserContextForEyeoFactories(
      content::BrowserContext* current_browser_context) override;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_ADBLOCK_ADBLOCK_AW_CONTENT_BROWSER_CLIENT_H_
