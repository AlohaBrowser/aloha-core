// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Aloha Mobile Ltd.

#include "content/public/browser/media_player_id.h"

namespace content {

// ALOHA https://app.clickup.com/t/2f2eyt8
MediaPlayerId::MediaPlayerId(GlobalRenderFrameHostId frame_routing_id,
                             int player_id, const media::mojom::MediaPlayerId& aloha_player_id)
			: frame_routing_id(frame_routing_id), player_id(player_id), aloha_player_id(aloha_player_id)  {}

// ALOHA https://app.clickup.com/t/2f2eyt8
MediaPlayerId MediaPlayerId::CreateMediaPlayerIdForTests() {
  return MediaPlayerId(GlobalRenderFrameHostId(), 0, {});
}

bool MediaPlayerId::operator==(const MediaPlayerId& other) const {
  bool ret_val = frame_routing_id == other.frame_routing_id &&
         player_id == other.player_id;
  // ALOHA https://app.clickup.com/t/2f2eyt8
  // if mediaplayer ids are equal - html_id should be equal as well
  if (ret_val) {
    DCHECK(aloha_player_id.html_id == other.aloha_player_id.html_id);
    DCHECK(aloha_player_id.iframes_ids == other.aloha_player_id.iframes_ids);
  } else {
    DCHECK(aloha_player_id.html_id != other.aloha_player_id.html_id ||
      aloha_player_id.iframes_ids != other.aloha_player_id.iframes_ids);
  }
  return ret_val;
}

bool MediaPlayerId::operator!=(const MediaPlayerId& other) const {
  // ALOHA https://app.clickup.com/t/2f2eyt8
  return !(*this == other);
}

bool MediaPlayerId::operator<(const MediaPlayerId& other) const {
  if (frame_routing_id == other.frame_routing_id)
    return player_id < other.player_id;
  return frame_routing_id < other.frame_routing_id;
}

}  // namespace content
