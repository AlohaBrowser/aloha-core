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

#include "media/mojo/mojom/media_player.mojom.h"

namespace content {
class WebContents;
}

namespace aloha {

struct BromiumClientBridge {
  static BromiumClientBridge* FromWebContents(content::WebContents* web_contents);

  virtual ~BromiumClientBridge() = default;

  // ALOHA https://app.clickup.com/t/2f2eyt8
  virtual void OnMediaPlay(
      const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url,
      const std::string& document_url, double duration_s, bool is_audio_only) = 0;
  // ALOHA https://app.clickup.com/t/2qfa6r7
  virtual void OnMediaPause(
      const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url,
      const std::string& document_url, double duration_s, bool is_audio_only) = 0;
  virtual void OnMediaDestroy(
      const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url,
      const std::string& document_url) = 0;
  // ALOHA https://app.clickup.com/t/2rqdtxz
  virtual void OnMediaError(
      const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
      const std::string& pipeline_status, const std::string& media_url,
      const std::string& document_url, double current_time_s, double duration_s) = 0;

  // ALOHA https://app.clickup.com/t/2hxwa9w
  virtual bool ShouldHideControlsInFullscreen(
      const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
      const std::string& url, double duration_s,
      const std::string& pending_elem_class) = 0;
  virtual void OnVideoFullscreenChanged(
      bool is_fullscreen, const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
      const std::string& url, bool media_controls_is_hidden) = 0;

  // ALOHA https://app.clickup.com/t/861md9r6t 
  virtual void OnInternalDownloadStarted(const std::string& http_method, int size_in_bytes) = 0;

  // ALOHA https://app.clickup.com/t/86epnk66e
  virtual bool ShouldPlayBackgroundVideo() = 0;
};

}  // namespace aloha
