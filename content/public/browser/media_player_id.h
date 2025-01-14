// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Aloha Mobile Ltd.

#ifndef CONTENT_PUBLIC_BROWSER_MEDIA_PLAYER_ID_H_
#define CONTENT_PUBLIC_BROWSER_MEDIA_PLAYER_ID_H_

#include "content/common/content_export.h"
#include "content/public/browser/global_routing_id.h"

// ALOHA https://app.clickup.com/t/2f2eyt8
#include "media/mojo/mojom/media_player.mojom.h"

namespace content {

struct CONTENT_EXPORT MediaPlayerId {
  static MediaPlayerId CreateMediaPlayerIdForTests();
  MediaPlayerId() = delete;

  // ALOHA https://app.clickup.com/t/2f2eyt8
  MediaPlayerId(GlobalRenderFrameHostId routing_id, int delegate_id,
    const media::mojom::MediaPlayerId& aloha_player_id = {});
  bool operator==(const MediaPlayerId&) const;
  bool operator!=(const MediaPlayerId&) const;
  bool operator<(const MediaPlayerId&) const;

  GlobalRenderFrameHostId frame_routing_id;
  int delegate_id;

  // ALOHA https://app.clickup.com/t/2f2eyt8
  media::mojom::MediaPlayerId aloha_player_id;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MEDIA_PLAYER_ID_H_
