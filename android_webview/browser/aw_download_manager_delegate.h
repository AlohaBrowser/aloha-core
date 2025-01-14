// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Aloha Mobile Ltd.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_DOWNLOAD_MANAGER_DELEGATE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>

#include "base/supports_user_data.h"
#include "content/public/browser/download_manager_delegate.h"

// ALOHA https://app.clickup.com/t/mz8wrn
#include "base/files/file_path.h"

namespace content {

class WebContents;

}  // namespace content

namespace android_webview {

// Android WebView does not use Chromium downloads, so implement methods here to
// unconditionally cancel the download.
class AwDownloadManagerDelegate : public content::DownloadManagerDelegate,
                                  public base::SupportsUserData::Data {
 public:
  AwDownloadManagerDelegate();

  AwDownloadManagerDelegate(const AwDownloadManagerDelegate&) = delete;
  AwDownloadManagerDelegate& operator=(const AwDownloadManagerDelegate&) =
      delete;

  ~AwDownloadManagerDelegate() override;

  // content::DownloadManagerDelegate implementation.
  bool InterceptDownloadIfApplicable(
      const GURL& url,
      const std::string& user_agent,
      const std::string& method, // ALOHA https://app.clickup.com/t/mz8wrn
      const bool is_disposable_url, // ALOHA https://app.clickup.com/t/861md9r6t
      const std::string& content_disposition,
      const std::string& mime_type,
      const std::string& request_origin,
      int64_t content_length,
      bool is_transient,
      content::WebContents* web_contents) override;

  // ALOHA https://app.clickup.com/t/mz8wrn
  bool DetermineDownloadTarget(download::DownloadItem* item,
                               download::DownloadTargetCallback* callback) override;
  // ALOHA https://app.clickup.com/t/mz8wrn
  bool ShouldOpenDownload(
    download::DownloadItem* item,
    content::DownloadOpenDelayedCallback callback) override;

 private:
  // ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
  const base::FilePath temporary_downloads_dir_;
  std::string last_aw_user_agent_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_DOWNLOAD_MANAGER_DELEGATE_H_
