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

#include "components/adblock/content/renderer/adblock_render_frame_observer.h"

#include "content/public/common/isolated_world_ids.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace adblock {

AdblockRenderFrameObserver::AdblockRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

AdblockRenderFrameObserver::~AdblockRenderFrameObserver() = default;

void AdblockRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != content::ISOLATED_WORLD_ID_ADBLOCK) {
    return;
  }

  if (!element_hide_api_) {
    element_hide_api_ = std::make_unique<ElementHideApi>(render_frame());
  }
  element_hide_api_->AddJavaScriptObjectToFrame(context);
}

void AdblockRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace adblock
