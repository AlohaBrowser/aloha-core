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
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "player/player_base.h"
#include "player/type.h"
// https://app.clickup.com/t/86eppm2e8

#include "base/no_destructor.h"
namespace aloha {

class FullscreenStateHandler {
 public:

  static FullscreenStateHandler* GetInstance();

  FullscreenStateHandler(const FullscreenStateHandler&) = delete;
  FullscreenStateHandler& operator=(const FullscreenStateHandler&) = delete;

  void PrepareForFullscreen(blink::Element* video_elem, const std::string& url);
  void Reset();
  bool IgnoreExitFullscreen(blink::ScriptState* script_state, blink::ExceptionState* exception_state);
  player::PlayerType GetType();

  void ExitFullscreen(blink::Element* video_elem);

 private:
  friend class base::NoDestructor<FullscreenStateHandler>;
  std::unique_ptr<player::PlayerBase> player_{nullptr};

  FullscreenStateHandler();
  ~FullscreenStateHandler();
};

} // namespace aloha
