// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Aloha Mobile Ltd.

#include "android_webview/browser/aw_download_manager_delegate.h"

#include "android_webview/browser/aw_content_browser_client.h"
#include "android_webview/browser/aw_contents_client_bridge.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

// ALOHA https://app.clickup.com/t/mz8wrn
#include "aloha/src/native/bromium.h"
#include "base/uuid.h"
#include "base/files/file_util.h"
#include "content/public/browser/download_item_utils.h"
#include "components/download/public/common/download_item.h"
#include "components/download/public/common/download_task_runner.h"
// ALOHA https://app.clickup.com/t/2u59j0h
#include "aloha/src/native/aloha_consts.h"

namespace android_webview {

// ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
AwDownloadManagerDelegate::AwDownloadManagerDelegate() :
    temporary_downloads_dir_(aloha::GetTemporaryDownloadsDir()) {
  download::GetDownloadTaskRunner()->PostTask(
    FROM_HERE,
    base::BindOnce(
      base::IgnoreResult(base::CreateDirectory),
      temporary_downloads_dir_));
}

AwDownloadManagerDelegate::~AwDownloadManagerDelegate() = default;

bool AwDownloadManagerDelegate::InterceptDownloadIfApplicable(
    const GURL& url,
    const std::string& user_agent,
    const std::string& method, // ALOHA https://app.clickup.com/t/mz8wrn
    const bool is_disposable_url, // ALOHA https://app.clickup.com/t/861md9r6t
    const std::string& content_disposition,
    const std::string& mime_type,
    const std::string& request_origin,
    int64_t content_length,
    bool is_transient,
    content::WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!web_contents)
    return true;

  AwContentsClientBridge* client =
      AwContentsClientBridge::FromWebContents(web_contents);
  if (!client)
    return true;

  std::string aw_user_agent =
      web_contents->GetUserAgentOverride().ua_string_override;
  if (aw_user_agent.empty()) {
    // use default user agent if nothing is provided
    aw_user_agent = user_agent.empty() ? GetUserAgent() : user_agent;
  }

  // ALOHA https://app.clickup.com/t/mz8wrn
  // Maybe need to check |content_disposition| contains 'attachment'.
  if (base::EqualsCaseInsensitiveASCII(method, "POST") ||
      (base::EqualsCaseInsensitiveASCII(method, "GET") && // ALOHA https://app.clickup.com/t/mz8wrn?comment=90040000622749
        base::StartsWith(url.possibly_invalid_spec(), "blob:", base::CompareCase::INSENSITIVE_ASCII)) ||
      (request_origin == aloha::kDownloadRequestOrigin && // ALOHA https://app.clickup.com/t/2u59j0h
        content_length < aloha::kHttpCacheMaxFileSizeBytes) ||
       is_disposable_url) { // ALOHA https://app.clickup.com/t/861md9r6t
    
    last_aw_user_agent_ = aw_user_agent;
    
    // ALOHA https://app.clickup.com/t/861md9r6t 
    if (auto* client_bromium = aloha::BromiumClientBridge::FromWebContents(web_contents); 
        client_bromium)
          client_bromium->OnInternalDownloadStarted(method, content_length); 
    
    // Use default download method for cached file or file included in body of POST response.
    return false;
  }

  client->NewDownload(url, url, // ALOHA https://app.clickup.com/t/861me45jv
                      aw_user_agent, content_disposition, mime_type,
                      {}, {}, // ALOHA https://app.clickup.com/t/mz8wrn
                      content_length);
  return true;
}

// ALOHA https://app.clickup.com/t/mz8wrn
bool AwDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* item, download::DownloadTargetCallback* callback) {
  auto file_path = temporary_downloads_dir_.Append(
      base::Uuid::GenerateRandomV4().AsLowercaseString());
  DCHECK(callback != nullptr);
  download::DownloadTargetInfo download_target_info_param;
  download_target_info_param.target_path = file_path;
  download_target_info_param.intermediate_path = file_path;
  download_target_info_param.mime_type = item->GetMimeType(); 
  std::move(*callback).Run(download_target_info_param);
  return true;
}

// ALOHA https://app.clickup.com/t/mz8wrn
bool AwDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* item,
    content::DownloadOpenDelayedCallback callback) {
  if (auto* web_contents = content::DownloadItemUtils::GetWebContents(item)) {
    if (auto* client = AwContentsClientBridge::FromWebContents(web_contents)) {
      client->NewDownload(item->GetURL(),
          item->GetOriginalUrl(), // ALOHA https://app.clickup.com/t/861me45jv
          last_aw_user_agent_,
          item->GetContentDisposition(), item->GetMimeType(),
          item->GetSuggestedFilename(),
          item->GetFullPath().value(), item->GetTotalBytes());
    }
  }
  return true;
}

}  // namespace android_webview
