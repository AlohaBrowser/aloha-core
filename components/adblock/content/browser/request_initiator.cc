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

#include "components/adblock/content/browser/request_initiator.h"

#include "base/check.h"
#include "content/public/browser/render_frame_host.h"

namespace adblock {

RequestInitiator::RequestInitiator(content::RenderFrameHost* render_frame_host)
    : initiator_(render_frame_host->GetGlobalId()) {}

RequestInitiator::RequestInitiator(GURL initiator_url)
    : initiator_(std::move(initiator_url)) {}

RequestInitiator::~RequestInitiator() = default;

RequestInitiator::RequestInitiator(const RequestInitiator& other) = default;

RequestInitiator& RequestInitiator::operator=(const RequestInitiator& other) =
    default;

RequestInitiator::RequestInitiator(RequestInitiator&& other) = default;

RequestInitiator& RequestInitiator::operator=(RequestInitiator&& other) =
    default;

bool RequestInitiator::IsFrame() const {
  return absl::holds_alternative<content::GlobalRenderFrameHostId>(initiator_);
}

content::RenderFrameHost* RequestInitiator::GetRenderFrameHost() const {
  DCHECK(IsFrame());
  return content::RenderFrameHost::FromID(
      absl::get<content::GlobalRenderFrameHostId>(initiator_));
}

const GURL& RequestInitiator::GetInitiatorUrl() const {
  DCHECK(!IsFrame());
  return absl::get<GURL>(initiator_);
}

}  // namespace adblock
