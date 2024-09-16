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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_OPENER_INFO_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_OPENER_INFO_H_

#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents_user_data.h"

namespace adblock {

class FrameOpenerInfo final
    : public content::WebContentsUserData<FrameOpenerInfo> {
 public:
  ~FrameOpenerInfo() final;

  content::GlobalRenderFrameHostId GetOpener() const;
  void SetOpener(content::GlobalRenderFrameHostId render_frame_host_id);

 private:
  explicit FrameOpenerInfo(content::WebContents* contents);

  content::GlobalRenderFrameHostId render_frame_host_id_ =
      content::GlobalRenderFrameHostId{MSG_ROUTING_NONE, MSG_ROUTING_NONE};

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_OPENER_INFO_H_
