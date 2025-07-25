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

#ifndef COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_
#define COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_

#include "components/adblock/content/renderer/adblock_render_frame_observer.h"
#include "content/public/renderer/content_renderer_client.h"

namespace adblock {

template <class ContentRendererClientBase>
class AdblockContentRendererClient : public ContentRendererClientBase {
 public:
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
};

template <class ContentRendererClientBase>
void AdblockContentRendererClient<ContentRendererClientBase>::
    RenderFrameCreated(content::RenderFrame* render_frame) {
  new AdblockRenderFrameObserver(render_frame);
  ContentRendererClientBase::RenderFrameCreated(render_frame);
}

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_
