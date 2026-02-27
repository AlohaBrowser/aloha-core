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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_IMPL_H_

#include <memory>
#include <string>
#include <tuple>

#include "absl/types/optional.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/net/adblock_resource_request.h"
#include "components/adblock/core/subscription/conversion_executors.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"

class PrefService;

namespace adblock {

class SubscriptionDownloaderImpl final : public SubscriptionDownloader {
 public:
  // Used to create AdblockResourceRequest to implement concurrent HEAD and
  // GET requests for subscriptions.
  using SubscriptionRequestMaker =
      base::RepeatingCallback<std::unique_ptr<AdblockResourceRequest>()>;

  SubscriptionDownloaderImpl(
      SubscriptionRequestMaker request_maker,
      ConversionExecutors* conversion_executor,
      SubscriptionPersistentMetadata* persistent_metadata);
  ~SubscriptionDownloaderImpl() final;
  void StartDownload(const GURL& subscription_url,
                     AdblockResourceRequest::RetryPolicy retry_policy,
                     DownloadCompletedCallback on_finished) final;
  void CancelDownload(const GURL& subscription_url) final;
  void DoHeadRequest(const GURL& subscription_url,
                     HeadRequestCallback on_finished) final;

  static constexpr int kMaxNumberOfRedirects = 5;

 private:
  using ResourceRequestPtr = std::unique_ptr<AdblockResourceRequest>;
  // Represents subscription downloads in progress.
  using OngoingDownload =
      std::tuple<ResourceRequestPtr, DownloadCompletedCallback>;
  using OngoingDownloads = std::map<GURL, OngoingDownload>;
  using OngoingDownloadsIt = OngoingDownloads::iterator;
  // There's never more than one concurrent HEAD request - for the
  // Acceptable Ads subscription, a special case in user counting. This will
  // be replaced by a dedicated solution for user counting (Telemetry)
  // eventually.
  using HeadRequest = std::tuple<ResourceRequestPtr, HeadRequestCallback>;

  bool IsUrlAllowed(const GURL& subscription_url) const;
  bool IsConnectionAllowed() const;
  void OnDownloadFinished(const GURL& subscription_url,
                          base::FilePath downloaded_file,
                          scoped_refptr<net::HttpResponseHeaders> headers);
  void OnHeadersOnlyDownloaded(const GURL& subscription_url,
                               base::FilePath downloaded_file,
                               scoped_refptr<net::HttpResponseHeaders> headers);
  void OnConversionFinished(const GURL& subscription_url,
                            ConversionResult converter_result);
  void AbortWithWarning(const OngoingDownloadsIt ongoing_download_it,
                        const std::string& warning);

  SEQUENCE_CHECKER(sequence_checker_);
  SubscriptionRequestMaker request_maker_;
  raw_ptr<ConversionExecutors> conversion_executor_;
  raw_ptr<SubscriptionPersistentMetadata> persistent_metadata_;
  OngoingDownloads ongoing_downloads_;
  absl::optional<HeadRequest> ongoing_ping_;
  base::WeakPtrFactory<SubscriptionDownloaderImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_DOWNLOADER_IMPL_H_
