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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_DOCUMENT_INFO_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_DOCUMENT_INFO_H_

#include "content/public/browser/document_user_data.h"
#include "url/gurl.h"

namespace adblock {

class EyeoDocumentInfo final
    : public content::DocumentUserData<EyeoDocumentInfo> {
 public:
  ~EyeoDocumentInfo() final;

  // Returns a URL usable for building the frame hierarchy.
  // Returns the pre-commit URL if it was set, otherwise the last committed URL
  // of the RenderFrameHost. This resolves a race condition where depending on
  // RenderFrameHost::GetLastCommitedURL() alone could return an empty URL if
  // the navigation was still in progress.
  const GURL& GetURL() const;
  // Set the URL of the document, as soon as it becomes known - accounting for
  // possible redirects. This EyeoDocumentInfo will be destroyed and re-created
  // when the navigation commits, so the value set here is temporary - like the
  // entire object.
  void SetPreCommitURL(GURL url);

  bool IsElementHidingDone() const;
  void SetElementHidingDone();

 private:
  explicit EyeoDocumentInfo(content::RenderFrameHost* rfh);

  bool element_hiding_done_ = false;
  GURL pre_commit_url_;

  friend DocumentUserData;
  DOCUMENT_USER_DATA_KEY_DECL();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_DOCUMENT_INFO_H_
