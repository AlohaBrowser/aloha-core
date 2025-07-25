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

#include "fullscreen_state_handler.h"

namespace aloha {

namespace
{
    const char* kJwPlayer = "jw-";
} // namespace anonymous

void FullscreenStateHandler::PrepareForFullscreen(blink::Element* video_elem) {
    if(!video_elem) {
        return;
    }    
    auto class_name = video_elem->getAttribute(AtomicString("class"));
    is_jwplayer_ = class_name.Contains(kJwPlayer);
}

void FullscreenStateHandler::Reset() {
    is_jwplayer_ = false;
}

bool FullscreenStateHandler::IgnoreExitFullscreen(blink::ScriptState* script_state, blink::ExceptionState* exception_state) { 
   return (is_jwplayer_ && 
            script_state != nullptr &&
                 exception_state != nullptr );
}

}  // namespace aloha
