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

FullscreenStateHandler* FullscreenStateHandler::GetInstance() {
  static base::NoDestructor<FullscreenStateHandler> instance;
  return instance.get();
}

FullscreenStateHandler::FullscreenStateHandler() = default;
FullscreenStateHandler::~FullscreenStateHandler() = default;

void FullscreenStateHandler::PrepareForFullscreen(blink::Element* video_elem, const std::string& url) {
  player_ = player::GetPlayer(video_elem, url);
  if (!player_) {
    return;
  }
  player_->EnterFullscreen(video_elem);
}

void FullscreenStateHandler::Reset() {
  if (player_) {
    player_->Reset();
  }
  player_.reset();
}

bool FullscreenStateHandler::IgnoreExitFullscreen(blink::ScriptState* script_state, blink::ExceptionState* exception_state) {
  if (player_) {
    return player_->IgnoreExitFullscreen(script_state, exception_state);
  }
  return false;
}

player::PlayerType FullscreenStateHandler::GetType() {
  if (player_) {
    return player_->GetType();
  }
  return player::PlayerType::kUnknown;
}

void FullscreenStateHandler::ExitFullscreen(blink::Element* video_elem) {
  if (player_) {
    player_->ExitFullscreen(video_elem);
  }
}

}  // namespace aloha
