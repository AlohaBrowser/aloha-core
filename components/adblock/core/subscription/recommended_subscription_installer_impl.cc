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

#include "components/adblock/core/subscription/recommended_subscription_installer_impl.h"

#include "base/functional/callback.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/recommended_subscription_parser.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_response_headers.h"

namespace adblock {
namespace {
// Subscriptions that are not recommended for 14 days are removed
constexpr base::TimeDelta kAutoInstalledSubscriptionExpirationInterval =
    base::Days(14);
// Auto installed subscriptions are updated every day
constexpr base::TimeDelta kAutoInstalledSubscriptionUpdateInterval =
    base::Days(1);
}  // namespace

RecommendedSubscriptionInstallerImpl::RecommendedSubscriptionInstallerImpl(
    PrefService* pref_service,
    FilteringConfiguration* configuration,
    SubscriptionPersistentMetadata* persistent_metadata,
    ResourceRequestMaker request_maker)
    : pref_service_(pref_service),
      configuration_(configuration),
      persistent_metadata_(persistent_metadata),
      request_maker_(std::move(request_maker)) {
  DCHECK(configuration_->GetName() == kAdblockFilteringConfigurationName)
      << "Recommended subscriptions should only be installed for adblock "
         "configuration";
}

RecommendedSubscriptionInstallerImpl::~RecommendedSubscriptionInstallerImpl() {}

void RecommendedSubscriptionInstallerImpl::RunUpdateCheck() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (IsUpdateDue()) {
    VLOG(1) << "[eyeo] Running recommended subscription update";
    resource_request_ = request_maker_.Run();
    resource_request_->Start(
        RecommendedSubscriptionListUrl(), AdblockResourceRequest::Method::GET,
        base::BindRepeating(&RecommendedSubscriptionInstallerImpl::
                                OnRecommendationListDownloaded,
                            weak_ptr_factory_.GetWeakPtr()),
        AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, "");
  }
}

bool RecommendedSubscriptionInstallerImpl::IsUpdateDue() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return pref_service_->GetBoolean(
             common::prefs::kEnableAutoInstalledSubscriptions) &&
         pref_service_->GetTime(
             common::prefs::kAutoInstalledSubscriptionsNextUpdateTime) <=
             base::Time::Now();
}

void RecommendedSubscriptionInstallerImpl::OnRecommendationListDownloaded(
    const GURL& url,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "[eyeo] Finished downloading recommended subscription list";

  resource_request_.reset();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&RecommendedSubscriptionParser::FromFile, downloaded_file),
      base::BindOnce(&RecommendedSubscriptionInstallerImpl::
                         OnRecommendedSubscriptionsParsed,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RecommendedSubscriptionInstallerImpl::OnRecommendedSubscriptionsParsed(
    const std::vector<GURL>& recommended_subscriptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& subscription : recommended_subscriptions) {
    VLOG(1) << "[eyeo] Adding auto installed subscription: " << subscription;

    if (configuration_->IsFilterListPresent(subscription) &&
        !persistent_metadata_->IsAutoInstalled(subscription)) {
      VLOG(1) << "[eyeo] Skipping recommended subscription since it's already "
                 "added as not auto installed";
      continue;
    }

    // If the list is not present already, subscribe to it.
    // Adding existing list is NOP so there is no need to check.
    configuration_->AddFilterList(subscription);

    persistent_metadata_->SetAutoInstalledExpirationInterval(
        subscription, kAutoInstalledSubscriptionExpirationInterval);
  }

  for (const auto& filter_list : configuration_->GetFilterLists()) {
    // Remove auto installed subscription if it's not recommended for a while
    if (persistent_metadata_->IsAutoInstalledExpired(filter_list)) {
      VLOG(1) << "[eyeo] Removing auto installed subscription: " << filter_list;
      configuration_->RemoveFilterList(filter_list);
    }
  }

  pref_service_->SetTime(
      common::prefs::kAutoInstalledSubscriptionsNextUpdateTime,
      base::Time::Now() + kAutoInstalledSubscriptionUpdateInterval);
}

void RecommendedSubscriptionInstallerImpl::RemoveAutoInstalledSubscriptions() {
  for (const auto& filter_list : configuration_->GetFilterLists()) {
    // Remove auto installed subscription if it's not recommended for a while
    if (persistent_metadata_->IsAutoInstalled(filter_list)) {
      VLOG(1) << "[eyeo] Removing auto installed subscription: " << filter_list;
      configuration_->RemoveFilterList(filter_list);
    }
  }
}

}  // namespace adblock
