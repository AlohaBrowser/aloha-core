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
GURL GetUrlAsReferrer(content::RenderFrameHost* frame_host) {
  EyeoDocumentInfo* document_info =
      EyeoDocumentInfo::GetOrCreateForCurrentDocument(frame_host);
  const GURL& frame_url = document_info->GetURL();
  if (frame_url.IsAboutBlank() && IsValidForFrameHierarchy(frame_url)) {
    return frame_url;
  }
  return frame_url.GetAsReferrer();
}

}  // namespace

FrameHierarchyBuilder::FrameHierarchyBuilder() = default;

FrameHierarchyBuilder::~FrameHierarchyBuilder() = default;

content::RenderFrameHost* FrameHierarchyBuilder::FindRenderFrameHost(
    content::GlobalRenderFrameHostId render_frame_host_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return content::RenderFrameHost::FromID(render_frame_host_id);
}

std::vector<GURL> FrameHierarchyBuilder::BuildFrameHierarchy(
    content::RenderFrameHost* host) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(host) << "RenderFrameHost is needed to build frame hierarchy";

  std::vector<GURL> referrers_chain;
  for (auto* iter = host; iter; iter = iter->GetParent()) {
    auto last_commited_referrer = GetUrlAsReferrer(iter);
    if (IsValidForFrameHierarchy(last_commited_referrer)) {
      VLOG(2) << "[eyeo] FrameHierarchy item: "
              << last_commited_referrer.spec();
      referrers_chain.emplace_back(last_commited_referrer);
    }
  }

  return referrers_chain;
}

GURL FrameHierarchyBuilder::FindUrlForFrame(
    content::RenderFrameHost* frame_host,
    content::WebContents* web_contents) const {
  GURL url = frame_host->GetLastCommittedURL();
  // Anonymous frames have "about:blank" source so we use a parent frame URL
  // Do not use GURL::IsExactlyAboutBlank (DPD-1946).
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
