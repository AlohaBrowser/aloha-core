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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_H_

#include <vector>

#include "base/functional/callback.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "url/gurl.h"

namespace adblock {

// Downloads filter lists from the Internet and converts them into flatbuffers.
// See also: OngoingSubscriptionRequest for more details about allowing and
// retrying downloads.
class SubscriptionDownloader {
 public:
  using DownloadCompletedCallback =
      base::OnceCallback<void(std::unique_ptr<FlatbufferData>)>;

  // For head requests we only need the parsed version as result
  using HeadRequestCallback = base::OnceCallback<void(const std::string)>;
  // Controls retry behavior when download or conversion failed.
  enum class RetryPolicy {
    // Will retry with a progressive back-off until download succeeded.
    RetryUntilSucceeded,
    // Will only try to download and convert the subscription once.
    DoNotRetry,
  };
  virtual ~SubscriptionDownloader() = default;
  // Starts downlading |subscription_url|. |on_finished| will be called with
  // the converted flatbuffer. |retry_policy| controls failure-handling
  // behavior. If downloading is disallowed due to current network state, it is
  // deferred until conditions allow it.
  virtual void StartDownload(const GURL& subscription_url,
                             RetryPolicy retry_policy,
                             DownloadCompletedCallback on_finished) = 0;
  // Cancels ongoing downloads for matching |url|, including retry attempts or
  // downloads deferred due to network conditions.
  virtual void CancelDownload(const GURL& subscription_url) = 0;
  // Triggers head request on |subscription_url|. |on_finished| will be called
  // with the parsed date from response headers, or with the empty string if
  // the request was not successful.
  virtual void DoHeadRequest(const GURL& subscription_url,
                             HeadRequestCallback on_finished) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_H_
