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

#include "components/adblock/content/browser/frame_opener_info.h"

namespace adblock {

WEB_CONTENTS_USER_DATA_KEY_IMPL(FrameOpenerInfo);

FrameOpenerInfo::FrameOpenerInfo(content::WebContents* contents)
    : content::WebContentsUserData<FrameOpenerInfo>(*contents) {}

FrameOpenerInfo::~FrameOpenerInfo() = default;

content::GlobalRenderFrameHostId FrameOpenerInfo::GetOpener() const {
  return render_frame_host_id_;
}
void FrameOpenerInfo::SetOpener(
    content::GlobalRenderFrameHostId render_frame_host_id) {
  render_frame_host_id_ = render_frame_host_id;
}

}  // namespace adblock
