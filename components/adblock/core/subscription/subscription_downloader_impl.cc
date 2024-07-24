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

#include "components/adblock/core/subscription/subscription_downloader_impl.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"

namespace adblock {
namespace {

// To retain user anonymity, we clamp the download count sent to subscription
// servers - any number higher than 4 is reported as just "4+".
std::string GetClampedDownloadCount(int download_count) {
  DCHECK_GE(download_count, 0);
  if (download_count > 4) {
    return "4+";
  }
  return base::NumberToString(download_count);
}

// Returns a version of |subscription_url| with added GET parameters that
// describe client and request metadata.
GURL AddUrlParameters(const GURL& subscription_url,
                      const SubscriptionPersistentMetadata* persistent_metadata,
                      const utils::AppInfo& client_metadata,
                      const bool is_disabled) {
  const std::string query = base::StrCat(
      {"addonName=", "eyeo-chromium-sdk", "&addonVersion=", "2.0.0",
       "&application=", base::EscapeQueryParamValue(client_metadata.name, true),
       "&applicationVersion=",
       base::EscapeQueryParamValue(client_metadata.version, true), "&platform=",
       base::EscapeQueryParamValue(client_metadata.client_os, true),
       "&platformVersion=", "1.0", "&lastVersion=",
       base::EscapeQueryParamValue(
           persistent_metadata->GetVersion(subscription_url), true),
       "&disabled=", is_disabled ? "true" : "false", "&downloadCount=",
       GetClampedDownloadCount(
           persistent_metadata->GetDownloadSuccessCount(subscription_url))});

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return subscription_url.ReplaceComponents(replacements);
}

int GenerateTraceId(const GURL& subscription_url) {
  return std::hash<std::string>{}(subscription_url.spec());
}

}  // namespace

SubscriptionDownloaderImpl::SubscriptionDownloaderImpl(
    utils::AppInfo client_metadata,
    SubscriptionRequestMaker request_maker,
    ConversionExecutors* conversion_executor,
    SubscriptionPersistentMetadata* persistent_metadata)
    : client_metadata_(std::move(client_metadata)),
      request_maker_(std::move(request_maker)),
      conversion_executor_(conversion_executor),
      persistent_metadata_(persistent_metadata) {}

SubscriptionDownloaderImpl::~SubscriptionDownloaderImpl() = default;

void SubscriptionDownloaderImpl::StartDownload(
    const GURL& subscription_url,
    RetryPolicy retry_policy,
    DownloadCompletedCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsUrlAllowed(subscription_url)) {
    LOG(WARNING) << "[eyeo] Download from URL not allowed, will not download "
                 << subscription_url;
    std::move(on_finished).Run(nullptr);
    return;
  }

  ongoing_downloads_[subscription_url] = OngoingDownload{
      request_maker_.Run(), retry_policy, std::move(on_finished)};
  std::get<OngoingRequestPtr>(ongoing_downloads_[subscription_url])
      ->Start(
          AddUrlParameters(subscription_url, persistent_metadata_,
                           client_metadata_, false),
          OngoingSubscriptionRequest::Method::GET,
          base::BindRepeating(&SubscriptionDownloaderImpl::OnDownloadFinished,
                              weak_ptr_factory_.GetWeakPtr()));
}

void SubscriptionDownloaderImpl::CancelDownload(const GURL& subscription_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ongoing_downloads_.erase(subscription_url);
}

void SubscriptionDownloaderImpl::DoHeadRequest(
    const GURL& subscription_url,
    HeadRequestCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ongoing_ping_) {
    std::move(std::get<HeadRequestCallback>(*ongoing_ping_)).Run("");
  }

  ongoing_ping_ = HeadRequest{request_maker_.Run(), std::move(on_finished)};
  std::get<OngoingRequestPtr>(*ongoing_ping_)
      ->Start(AddUrlParameters(subscription_url, persistent_metadata_,
                               client_metadata_, true),
              OngoingSubscriptionRequest::Method::HEAD,
              base::BindRepeating(
                  &SubscriptionDownloaderImpl::OnHeadersOnlyDownloaded,
                  weak_ptr_factory_.GetWeakPtr()));
}

bool SubscriptionDownloaderImpl::IsUrlAllowed(
    const GURL& subscription_url) const {
  if (net::IsLocalhost(subscription_url)) {
    // We trust all localhost urls, regardless of scheme.
    return true;
  }
  if (!subscription_url.SchemeIs("https") &&
      !subscription_url.SchemeIs("data")) {
    return false;
  }
  return true;
}

void SubscriptionDownloaderImpl::OnHeadersOnlyDownloaded(
    const GURL&,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(ongoing_ping_.has_value());
  base::Time date;
  std::string version("");

  // Parse date or Date from response headers.
  if (headers && headers->GetDateValue(&date)) {
    version = utils::ConvertBaseTimeToABPFilterVersionFormat(date);
  }

  std::move(std::get<HeadRequestCallback>(*ongoing_ping_))
      .Run(std::move(version));
  ongoing_ping_.reset();
  // A ping may fail. We don't retry, SubscriptionUpdater will send a new ping
  // in an hour anyway.
}

void SubscriptionDownloaderImpl::OnDownloadFinished(
    const GURL& subscription_url,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto download_it = ongoing_downloads_.find(subscription_url);
  DCHECK(download_it != ongoing_downloads_.end());

  // For compatibility with existing Appium tests. To be removed after DPD-808.
  if (headers) {
    LOG(INFO) << "[eyeo] Response status for "
              << AddUrlParameters(subscription_url, persistent_metadata_,
                                  client_metadata_, false)
              << " : " << headers->response_code();
  }

  if (downloaded_file.empty()) {
    persistent_metadata_->IncrementDownloadErrorCount(subscription_url);
    if (std::get<RetryPolicy>(download_it->second) ==
        RetryPolicy::RetryUntilSucceeded) {
      DLOG(WARNING) << "[eyeo] Failed to retrieve content for "
                    << subscription_url << ", will retry";
      std::get<OngoingRequestPtr>(download_it->second)->Retry();
    } else {
      DLOG(WARNING) << "[eyeo] Failed to retrieve content for "
                    << subscription_url << ", will abort";
      std::move(std::get<DownloadCompletedCallback>(download_it->second))
          .Run(nullptr);
      ongoing_downloads_.erase(download_it);
    }
    return;
  }

  VLOG(1) << "[eyeo] Finished downloading " << subscription_url
          << ", starting conversion";

  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      "eyeo", "Converting subscription",
      TRACE_ID_LOCAL(GenerateTraceId(subscription_url)), "url",
      subscription_url.spec());

  conversion_executor_->ConvertFilterListFile(
      subscription_url, downloaded_file,
      base::BindOnce(&SubscriptionDownloaderImpl::OnConversionFinished,
                     weak_ptr_factory_.GetWeakPtr(), subscription_url));
}

void SubscriptionDownloaderImpl::OnConversionFinished(
    const GURL& subscription_url,
    ConversionResult converter_result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      "eyeo", "Converting subscription",
      TRACE_ID_LOCAL(GenerateTraceId(subscription_url)));
  const auto download_it = ongoing_downloads_.find(subscription_url);
  if (download_it == ongoing_downloads_.end()) {
    VLOG(1) << "[eyeo] Conversion result discarded, subscription download "
               "was cancelled.";
    return;
  }

  if (absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
          converter_result)) {
    VLOG(1) << "[eyeo] Finished converting " << subscription_url
            << " successfully";
    std::move(std::get<DownloadCompletedCallback>(download_it->second))
        .Run(std::move(
            absl::get<std::unique_ptr<FlatbufferData>>(converter_result)));
    ongoing_downloads_.erase(download_it);
  } else if (absl::holds_alternative<GURL>(converter_result)) {
    const GURL& redirect_url = absl::get<GURL>(converter_result);
    if (!IsUrlAllowed(redirect_url)) {
      AbortWithWarning(download_it, "Redirect URL not allowed.");
      return;
    }
    if (redirect_url == subscription_url) {
      AbortWithWarning(download_it,
                       "Redirect to the same URL is not permitted.");
      return;
    }
    if (std::get<OngoingRequestPtr>(download_it->second)
            ->GetNumberOfRedirects() >= kMaxNumberOfRedirects) {
      AbortWithWarning(download_it, "Maximum number of redirects exceeded.");
    } else {
      auto ongoing_download = ongoing_downloads_.extract(download_it);
      ongoing_download.key() = redirect_url;
      auto redirected_download_it =
          ongoing_downloads_.insert(std::move(ongoing_download)).position;
      std::get<OngoingRequestPtr>(redirected_download_it->second)
          ->Redirect(AddUrlParameters(redirect_url, persistent_metadata_,
                                      client_metadata_, false));
    }
  } else {
    persistent_metadata_->IncrementDownloadErrorCount(subscription_url);
    AbortWithWarning(download_it,
                     *absl::get<ConversionError>(converter_result));
    return;
  }
}

void SubscriptionDownloaderImpl::AbortWithWarning(
    const OngoingDownloadsIt ongoing_download_it,
    const std::string& warning) {
  if (ongoing_download_it == ongoing_downloads_.end()) {
    return;
  }
  DLOG(WARNING) << "[eyeo] " << warning << " Aborting download of "
                << ongoing_download_it->first;
  std::move(std::get<DownloadCompletedCallback>(ongoing_download_it->second))
      .Run(nullptr);
  ongoing_downloads_.erase(ongoing_download_it);
}

}  // namespace adblock
