/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_REQUEST_INITIATOR_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_REQUEST_INITIATOR_H_

#include "absl/types/variant.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "url/gurl.h"

namespace adblock {

// RequestInitiator is used to create a frame hierarchy for allowlisting that's
// relevant for a network request. It's the GlobalRenderFrameHostId that
// represents a frame/page that made the request, or the initiator URL of a
// request triggered from a detached service worker.
class RequestInitiator {
 public:
  // Appropriate for requests triggered from a frame, like a website loading an
  // image, or an iframe loading a script.
  explicit RequestInitiator(content::RenderFrameHost* render_frame_host);

  // Appropriate for requests triggered from a detached service worker. The
  // initiator URL is the URL of the service worker script. There is no
  // associated frame, a single service worker script can make requests on
  // behalf of multiple frames or even tabs.
  // |initiator_url| must be valid.
  explicit RequestInitiator(GURL initiator_url);

  ~RequestInitiator();
  RequestInitiator(const RequestInitiator&);
  RequestInitiator& operator=(const RequestInitiator&);
  RequestInitiator(RequestInitiator&&);
  RequestInitiator& operator=(RequestInitiator&&);

  bool operator==(const RequestInitiator& other) const = default;

  // Returns true if the initiator is a frame, false if it's a URL.
  bool IsFrame() const;

  // Asserts IsFrame().
  // The returned pointer may be null if the frame has been destroyed.
  content::RenderFrameHost* GetRenderFrameHost() const;

  // Asserts !IsFrame()
  const GURL& GetInitiatorUrl() const;

 private:
  absl::variant<content::GlobalRenderFrameHostId, GURL> initiator_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_REQUEST_INITIATOR_H_
