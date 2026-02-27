// Copyright 2025 Aloha Mobile Ltd.

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

#include <memory>
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "type.h"

namespace aloha::player {

 class PlayerBase;

 bool IsYouTube(const std::string& url);
 std::unique_ptr<PlayerBase> GetPlayer(blink::Element* element, const std::string& url);

 class PlayerBase {
  public:

   // Prepare element for fullscreen mode
   virtual void EnterFullscreen(blink::Element* element) = 0;
   // Restore element to native state
   virtual void ExitFullscreen(blink::Element* element) = 0;

   // For JW Player, we need to ignore exit fullscreen
   virtual bool IgnoreExitFullscreen(blink::ScriptState* script_state, blink::ExceptionState* exception_state) = 0;
   // Get the type of the player
   virtual PlayerType GetType() const = 0;
   // Reset the handler state between entering and exiting fullscreen
   // This is needed because entering and exiting fullscreen are static functions
   // and we need an object to store the intermediate state.
   virtual void Reset() = 0;

   virtual ~PlayerBase() = default;
 };
}