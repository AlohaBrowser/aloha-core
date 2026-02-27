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

#ifndef COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_RENDER_FRAME_OBSERVER_H_
#define COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_RENDER_FRAME_OBSERVER_H_

#include "components/adblock/content/renderer/element_hide_api.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"

namespace adblock {

class AdblockRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit AdblockRenderFrameObserver(content::RenderFrame* render_frame);
  ~AdblockRenderFrameObserver() override;

  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  std::unique_ptr<ElementHideApi> element_hide_api_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_RENDER_FRAME_OBSERVER_H_
