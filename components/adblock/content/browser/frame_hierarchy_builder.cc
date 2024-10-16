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

#include "components/adblock/content/browser/frame_hierarchy_builder.h"

#include <string_view>
#include <vector>

#include "base/logging.h"
#include "components/adblock/content/browser/eyeo_document_info.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"
#include "url/gurl.h"

namespace adblock {

namespace {

// Do not use GURL::IsAboutBlank for frame hierarchy, it returns true for valid
// src like `about:blank?eyeo=true`
bool IsExactlyAboutBlank(const GURL url) {
  static const std::string_view kAboutBlankUrl = "about:blank";
  return url == kAboutBlankUrl;
}

bool IsValidForFrameHierarchy(const GURL& url) {
  return !url.is_empty() && !IsExactlyAboutBlank(url) &&
         url != content::kBlockedURL;
}

// Basically calling `GetAsReferrer()` on url strips elements that are not
// supposed to be sent as HTTP referrer: username, password and ref fragment,
// and this is what we want for frame hierarchy urls.
// But for urls like `about:blank?eyeo=true` calling `GetAsReferrer()` returns
// an empty url which excludes it from frame hierarchy, so for `about:blank...`
// we fallback to the original URL.
// Also `GetAsReferrer()` returns an empty url for file scheme urls.
GURL GetUrlAsReferrer(content::RenderFrameHost* frame_host) {
  EyeoDocumentInfo* document_info =
      EyeoDocumentInfo::GetOrCreateForCurrentDocument(frame_host);
  const GURL& frame_url = document_info->GetURL();
  if ((frame_url.IsAboutBlank() && IsValidForFrameHierarchy(frame_url)) ||
      frame_url.SchemeIsFile()) {
    return frame_url;
  }
  return frame_url.GetAsReferrer();
}

std::vector<GURL> BuildFrameHierarchyForRenderFrameHost(
    content::RenderFrameHost* frame_host) {
  std::vector<GURL> frame_hierarchy;
  for (auto* iter = frame_host; iter; iter = iter->GetParent()) {
    auto last_commited_referrer = GetUrlAsReferrer(iter);
    if (IsValidForFrameHierarchy(last_commited_referrer)) {
      VLOG(2) << "[eyeo] FrameHierarchy item: "
              << last_commited_referrer.spec();
      frame_hierarchy.emplace_back(last_commited_referrer);
    }
  }
  return frame_hierarchy;
}

std::vector<GURL> BuildFrameHierarchyForRequestInitiator(
    const GURL& initiator_url) {
  std::vector<GURL> frame_hierarchy;
  if (IsValidForFrameHierarchy(initiator_url)) {
    frame_hierarchy.emplace_back(initiator_url);
  }
  return frame_hierarchy;
}

}  // namespace

FrameHierarchyBuilder::FrameHierarchyBuilder() = default;

FrameHierarchyBuilder::~FrameHierarchyBuilder() = default;

std::vector<GURL> FrameHierarchyBuilder::BuildFrameHierarchy(
    const RequestInitiator& request_initiator) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // frame_hierarchy is used for allowlisting, so we can check if any of the
  // parents of the requesting entity are allowlisted, in the event that the
  // request gets blocked. For "parentless" requests originating from service
  // workers, we fall back to their "initiator" URLs. This will be the URL of
  // the service worker script, not necessarily the website that hosts it, so
  // it's not ideal, but it's the best we can do.
  if (request_initiator.IsFrame()) {
    return BuildFrameHierarchyForRenderFrameHost(
        request_initiator.GetRenderFrameHost());
  } else {
    return BuildFrameHierarchyForRequestInitiator(
        request_initiator.GetInitiatorUrl());
  }
}

GURL FrameHierarchyBuilder::FindUrlForFrame(
    content::RenderFrameHost* frame_host,
    content::WebContents* web_contents) const {
  GURL url = frame_host->GetLastCommittedURL();
  // Anonymous frames have "about:blank" source so we use a parent frame URL
  // Do not use GURL::IsAboutBlank (DPD-1946).
  if (IsExactlyAboutBlank(url)) {
    for (auto* parent = frame_host->GetParent(); parent;
         parent = parent->GetParent()) {
      GURL parent_url = parent->GetLastCommittedURL();
      if (!parent_url.spec().empty() && !IsExactlyAboutBlank(parent_url)) {
        url = parent_url;
        break;
      }
    }
    // If we still cannot find parent url let's use top level navigation url
    if (IsExactlyAboutBlank(url)) {
      url = web_contents->GetLastCommittedURL();
    }
  }
  return url;
}

}  // namespace adblock
