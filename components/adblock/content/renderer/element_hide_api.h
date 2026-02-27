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

#ifndef COMPONENTS_ADBLOCK_CONTENT_RENDERER_ELEMENT_HIDE_API_H_
#define COMPONENTS_ADBLOCK_CONTENT_RENDERER_ELEMENT_HIDE_API_H_

#include <string>
#include <vector>

#include "components/adblock/content/common/element_hide_api.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace adblock {

class ElementHideApi {
 public:
  explicit ElementHideApi(content::RenderFrame* render_frame);
  ~ElementHideApi();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);

 private:
  // Make sure the mojo service is connected.
  void EnsureServiceConnected();

  void LogSelectors(int action, std::vector<std::string> selectors);

  raw_ptr<content::RenderFrame> render_frame_;
  mojo::Remote<mojom::ElementHideApi> element_hide_api_handler_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_RENDERER_ELEMENT_HIDE_API_H_
