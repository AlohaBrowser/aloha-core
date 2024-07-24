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

#include <functional>
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"

namespace aloha {

// ALOHA https://app.clickup.com/t/2f2ey18
// Finding video url in video-elements near |node| and intersecting with |point_in_viewport|.
blink::KURL FindVideoURL(const blink::Node* node, const gfx::Point& point_in_viewport);

// ALOHA https://app.clickup.com/t/2v1r9c4
// Finding image url in image-elements near |node| and intersecting with |point_in_viewport|.
blink::KURL FindImageURL(const blink::Node* node, const gfx::Point& point_in_viewport);

// ALOHA https://app.clickup.com/t/2f2ey18 and https://app.clickup.com/t/2hxwa9w
//       and https://app.clickup.com/t/2qfa6r7
// Getting 'src' attribute from |elem| or its descendants.
blink::KURL GetMediaSourceURL(const blink::Element& elem);

// ALOHA https://app.clickup.com/t/2hxwa9w
// Iterating by all video elements in descendants of |elem|.
// |processor| return true for break foreach.
void ForeachByVideoElements(blink::Element& elem,
  const std::function<bool (blink::HTMLVideoElement&)>& processor);

} // namespace aloha
