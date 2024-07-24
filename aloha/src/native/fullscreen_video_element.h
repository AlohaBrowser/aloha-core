// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink-forward.h"

namespace aloha {

// ALOHA https://app.clickup.com/t/2hxwa9w and https://app.clickup.com/t/2k0734w
struct FullscreenVideoElement {
  blink::Member<blink::HTMLVideoElement> video; // notnull
  blink::mojom::blink::FullscreenVideoElementInfoPtr info; // notnull

  FullscreenVideoElement(
    blink::HTMLVideoElement* in_video,
    blink::mojom::blink::FullscreenVideoElementInfoPtr in_info);
  FullscreenVideoElement(FullscreenVideoElement&&);
  FullscreenVideoElement& operator=(FullscreenVideoElement&&);
  ~FullscreenVideoElement();
};

} // namespace aloha
