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

#include "components/adblock/content/browser/eyeo_document_info.h"

#include "content/public/browser/render_frame_host.h"

namespace adblock {

DOCUMENT_USER_DATA_KEY_IMPL(EyeoDocumentInfo);

EyeoDocumentInfo::EyeoDocumentInfo(content::RenderFrameHost* rfh)
    : content::DocumentUserData<EyeoDocumentInfo>(rfh) {}

EyeoDocumentInfo::~EyeoDocumentInfo() = default;

const GURL& EyeoDocumentInfo::GetURL() const {
  if (!pre_commit_url_.is_empty()) {
    return pre_commit_url_;
  }
  return render_frame_host().GetLastCommittedURL();
}

void EyeoDocumentInfo::SetPreCommitURL(GURL url) {
  pre_commit_url_ = std::move(url);
}

bool EyeoDocumentInfo::IsElementHidingDone() const {
  return element_hiding_done_;
}

void EyeoDocumentInfo::SetElementHidingDone() {
  element_hiding_done_ = true;
}

}  // namespace adblock
