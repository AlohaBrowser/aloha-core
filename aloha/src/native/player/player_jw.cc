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

#include "player_jw.h"

namespace aloha::player {

void PlayerJW::EnterFullscreen(blink::Element* element) {
  // TODO: need to consider the possibility of moving style observer from fullscreen.cc
}

void PlayerJW::ExitFullscreen(blink::Element* element) {
}

bool PlayerJW::IgnoreExitFullscreen(blink::ScriptState* script_state, blink::ExceptionState* exception_state) {
  return (script_state != nullptr && exception_state != nullptr); // Disable exiting fullscreen when video_element is going to fullscreen
}

PlayerType PlayerJW::GetType() const {
  return PlayerType::kJwPlayer;
}

void PlayerJW::Reset() {
}

} // namespace aloha::player
