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

#include "components/adblock/core/subscription/filtering_configuration_maintainer_impl.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/trace_event/common/trace_event_common.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"

// TODO(mpawlowski): Remove in DPD-1154. This class should not need to know
// anything about particular subscriptions, it should be generic.
#include "components/adblock/core/subscription/subscription_config.h"

namespace adblock {
namespace {
constexpr base::TimeDelta kDefaultHeadRequestExpirationInterval = base::Days(1);

}  // namespace

class FilteringConfigurationMaintainerImpl::OngoingInstallation final
    : public Subscription {
 public:
  explicit OngoingInstallation(const GURL& url) : url_(url) {}

  GURL GetSourceUrl() const final { return url_; }
  std::string GetTitle() const final { return {}; }
  std::string GetCurrentVersion() const final { return {}; }
  InstallationState GetInstallationState() const final {
    return InstallationState::Installing;
  }
  base::Time GetInstallationTime() const final { return {}; }
  base::TimeDelta GetExpirationInterval() const final { return {}; }

 private:
  friend class base::RefCountedThreadSafe<OngoingInstallation>;
  ~OngoingInstallation() final = default;
  GURL url_;
};

FilteringConfigurationMaintainerImpl::FilteringConfigurationMaintainerImpl(
    FilteringConfiguration* configuration,
    std::unique_ptr<SubscriptionPersistentStorage> storage,
    std::unique_ptr<SubscriptionDownloader> downloader,
    std::unique_ptr<PreloadedSubscriptionProvider>
        preloaded_subscription_provider,
    std::unique_ptr<SubscriptionUpdater> updater,
    ConversionExecutors* conversion_executor,
    SubscriptionPersistentMetadata* persistent_metadata,
    SubscriptionUpdatedCallback subscription_updated_callback)
    : configuration_(std::move(configuration)),
      storage_(std::move(storage)),
      downloader_(std::move(downloader)),
      preloaded_subscription_provider_(
          std::move(preloaded_subscription_provider)),
      updater_(std::move(updater)),
      conversion_executor_(conversion_executor),
      persistent_metadata_(persistent_metadata),
      subscription_updated_callback_(std::move(subscription_updated_callback)) {
  DCHECK(configuration_->IsEnabled())
      << "Disabled configurations should not be maintained";
  configuration_->AddObserver(this);
}

FilteringConfigurationMaintainerImpl::~FilteringConfigurationMaintainerImpl() {
  configuration_->RemoveObserver(this);
}

std::unique_ptr<SubscriptionCollection>
FilteringConfigurationMaintainerImpl::GetSubscriptionCollection() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<scoped_refptr<InstalledSubscription>> state = current_state_;
  if (custom_filters_) {
    state.push_back(custom_filters_);
  }
  base::ranges::move(
      preloaded_subscription_provider_->GetCurrentPreloadedSubscriptions(),
      std::back_inserter(state));
  VLOG(2) << "[eyeo] FilteringConfiguration " << configuration_->GetName()
          << " produces " << state.size() << " subscriptions for Snapshot";
  return std::make_unique<SubscriptionCollectionImpl>(
      state, configuration_->GetName());
}

std::vector<scoped_refptr<Subscription>>
FilteringConfigurationMaintainerImpl::GetCurrentSubscriptions() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Result will contain the currently installed subscriptions:
  std::vector<scoped_refptr<Subscription>> result;
  base::ranges::copy(current_state_, std::back_inserter(result));
  // And all preloaded subscriptions:
  auto preloaded_subscriptions =
      preloaded_subscription_provider_->GetCurrentPreloadedSubscriptions();
  base::ranges::move(preloaded_subscriptions, std::back_inserter(result));
  // Also, dummy subscriptions that represent ongoing installations (unless
  // already present, in which case they'd represent updates).
  base::ranges::copy_if(
      ongoing_installations_, std::back_inserter(result),
      [&](const auto& ongoing_installation) {
        return base::ranges::find(result, ongoing_installation->GetSourceUrl(),
                                  &Subscription::GetSourceUrl) == result.end();
      });
  return result;
}

void FilteringConfigurationMaintainerImpl::OnFilterListsChanged(
    FilteringConfiguration* config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(config, configuration_);
  if (status_ == StorageStatus::Initialized) {
    InstallMissingSubscriptions();
    RemoveUnneededSubscriptions();
  }
}

void FilteringConfigurationMaintainerImpl::OnAllowedDomainsChanged(
    FilteringConfiguration* config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(config, configuration_);
  OnCustomFiltersChanged(config);
}

void FilteringConfigurationMaintainerImpl::OnCustomFiltersChanged(
    FilteringConfiguration* config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(config, configuration_);
  SetCustomFilters();
}

bool FilteringConfigurationMaintainerImpl::IsInitialized() const {
  return status_ == StorageStatus::Initialized;
}

void FilteringConfigurationMaintainerImpl::InstallMissingSubscriptions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  // Subscriptions that are either installed or being installed:
  auto installed_subscriptions = GetReadySubscriptions();
  base::ranges::copy(GetPendingSubscriptions(),
                     std::back_inserter(installed_subscriptions));
  // Subscriptions that are demanded by the FilteringConfiguration:
  auto demanded_subscriptions = configuration_->GetFilterLists();
  base::ranges::sort(installed_subscriptions);
  base::ranges::sort(demanded_subscriptions);
  // Missing subscriptions is the difference between the two:
  std::vector<GURL> missing_subscriptions;
  base::ranges::set_difference(demanded_subscriptions, installed_subscriptions,
                               std::back_inserter(missing_subscriptions));
  for (const auto& url : missing_subscriptions) {
    DownloadAndInstallSubscription(url);
  }
}

void FilteringConfigurationMaintainerImpl::RemoveUnneededSubscriptions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  // Subscriptions that are either installed or being installed:
  auto installed_subscriptions = GetReadySubscriptions();
  base::ranges::copy(GetPendingSubscriptions(),
                     std::back_inserter(installed_subscriptions));
  // Subscriptions that are demanded by the FilteringConfiguration:
  auto demanded_subscriptions = configuration_->GetFilterLists();
  base::ranges::sort(installed_subscriptions);
  base::ranges::sort(demanded_subscriptions);
  installed_subscriptions.erase(std::unique(installed_subscriptions.begin(),
                                            installed_subscriptions.end()),
                                installed_subscriptions.end());
  // Unneeded subscriptions is the difference between the two:
  std::vector<GURL> unneeded_subscriptions;
  base::ranges::set_difference(installed_subscriptions, demanded_subscriptions,
                               std::back_inserter(unneeded_subscriptions));
  for (const auto& url : unneeded_subscriptions) {
    UninstallSubscription(url);
  }
}

void FilteringConfigurationMaintainerImpl::InitializeStorage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!IsInitialized());
  VLOG(1) << "[eyeo] FilteringConfigurationMaintainer starting.";
  TRACE_EVENT_ASYNC_BEGIN1(
      "eyeo", "FilteringConfigurationMaintainerImpl::InitializeStorage",
      TRACE_ID_LOCAL(this), "name", configuration_->GetName());
  storage_->LoadSubscriptions(
      base::BindOnce(&FilteringConfigurationMaintainerImpl::StorageInitialized,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FilteringConfigurationMaintainerImpl::StorageInitialized(
    std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!IsInitialized());
  DCHECK(current_state_.empty())
      << "current state was modified before initial state was loaded";
  current_state_ = std::move(loaded_subscriptions);
  status_ = StorageStatus::Initialized;
  // SubscriptionPersistentStorage allows multiple Subscriptions with same URL,
  // which is a legal transitive state during ex. installing an update.
  // However, current_state_ should always contain only
  // one subscription with a given URL. This normally happens automatically by
  // virtue of |SubscriptionAddedToStorage| calling |UninstallSubscription| but
  // this invariant might not hold if ex. the application exits after
  // SubscriptionPersistentStorage stores the update but before
  // SubscriptionServiceImpl uninstalls the old version. It's difficult to
  // make installing subscription updates atomic, so solve potential race
  // condition here:
  RemoveDuplicateSubscriptions();
  // Synchronize current state with the demands of the FilteringConfiguration:
  OnFilterListsChanged(configuration_);
  OnCustomFiltersChanged(configuration_);
  // Start periodic updates:
  updater_->StartSchedule(
      base::BindRepeating(&FilteringConfigurationMaintainerImpl::RunUpdateCheck,
                          weak_ptr_factory_.GetWeakPtr()));
  TRACE_EVENT_ASYNC_END1(
      "eyeo", "FilteringConfigurationMaintainerImpl::InitializeStorage",
      TRACE_ID_LOCAL(this), "name", configuration_->GetName());
}

void FilteringConfigurationMaintainerImpl::RemoveDuplicateSubscriptions() {
  // std::sort + std::unique is not good for this use case, as we need to
  // perform actions on the duplicates, not just discard them, and std::unique
  // leaves moved elements in unspecified state. std::adjacent_find or
  // std::unique_copy could be used as well, but using a helper std::set seems
  // simplest.
  const auto comparator = [](const auto& lhs, const auto& rhs) {
    return lhs->GetSourceUrl() < rhs->GetSourceUrl();
  };
  std::set<scoped_refptr<InstalledSubscription>, decltype(comparator)>
      unique_subscriptions(comparator);
  for (auto subscription : current_state_) {
    if (!unique_subscriptions.insert(subscription).second) {
      // This element already exists in the set, we found a duplicate.
      storage_->RemoveSubscription(subscription);
    }
  }
  current_state_.assign(unique_subscriptions.begin(),
                        unique_subscriptions.end());
}

void FilteringConfigurationMaintainerImpl::RunUpdateCheck() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "[eyeo] Running update check";
  for (auto& subscription : current_state_) {
    // Update subscriptions that are expired and aren't already in the process
    // of installing an update.
    const auto& url = subscription->GetSourceUrl();
    if (persistent_metadata_->IsExpired(url) &&
        base::ranges::find(ongoing_installations_, url,
                           &Subscription::GetSourceUrl) ==
            ongoing_installations_.end()) {
      VLOG(1) << "[eyeo] Updating expired subscription " << url;
      DownloadAndInstallSubscription(url);
    } else {
      VLOG(1) << "[eyeo] Skipping update of " << url << ": "
              << (!persistent_metadata_->IsExpired(url)
                      ? "not expired yet"
                      : "already downloading");
    }
  }
  // TODO(mpawlowski): remove after DPD-1154. If Acceptable Ads is not
  // installed, but it would have been expired, send HEAD request for Acceptable
  // Ads filter list just to count the user, without the intention of
  // downloading it.
  // This is to support legacy behavior.
  if (configuration_->GetName() == "adblock" &&
      base::ranges::none_of(GetCurrentSubscriptions(),
                            [](const auto& subscription) {
                              return subscription->GetSourceUrl() ==
                                     AcceptableAdsUrl();
                            }) &&
      persistent_metadata_->IsExpired(AcceptableAdsUrl())) {
    PingAcceptableAds();
  }
}

void FilteringConfigurationMaintainerImpl::DownloadAndInstallSubscription(
    const GURL& subscription_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  const bool is_an_update =
      base::ranges::any_of(current_state_, [&](const auto candidate) {
        return candidate->GetSourceUrl() == subscription_url;
      });

  // We do not retry downloading subscription updates, they will be retried
  // by the SubscriptionUpdater in due time anyway.
  auto retry_policy =
      is_an_update ? SubscriptionDownloader::RetryPolicy::DoNotRetry
                   : SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded;

  auto ongoing_installation =
      base::MakeRefCounted<OngoingInstallation>(subscription_url);
  ongoing_installations_.insert(ongoing_installation);
  UpdatePreloadedSubscriptionProvider();

  downloader_->StartDownload(
      subscription_url, retry_policy,
      base::BindOnce(
          &FilteringConfigurationMaintainerImpl::OnSubscriptionDataAvailable,
          weak_ptr_factory_.GetWeakPtr(), ongoing_installation));
}

void FilteringConfigurationMaintainerImpl::OnSubscriptionDataAvailable(
    scoped_refptr<OngoingInstallation> ongoing_installation,
    std::unique_ptr<FlatbufferData> raw_data) {
  if (ongoing_installations_.find(ongoing_installation) ==
      ongoing_installations_.end()) {
    // Installation was canceled.
    UpdatePreloadedSubscriptionProvider();
    return;
  }
  if (!raw_data) {
    // Download failed.
    ongoing_installations_.erase(ongoing_installation);
    UpdatePreloadedSubscriptionProvider();
    return;
  }

  storage_->StoreSubscription(
      std::move(raw_data),
      base::BindOnce(
          &FilteringConfigurationMaintainerImpl::SubscriptionAddedToStorage,
          weak_ptr_factory_.GetWeakPtr(), ongoing_installation));
}

void FilteringConfigurationMaintainerImpl::SubscriptionAddedToStorage(
    scoped_refptr<OngoingInstallation> ongoing_installation,
    scoped_refptr<InstalledSubscription> subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ongoing_installations_.find(ongoing_installation) ==
      ongoing_installations_.end()) {
    // Installation was canceled. We must now remove the subscription from
    // storage. Do not add it to |current_state|.
    storage_->RemoveSubscription(subscription);
    UpdatePreloadedSubscriptionProvider();
    return;
  }
  ongoing_installations_.erase(ongoing_installation);

  if (!subscription) {
    // There was an error adding subscription to storage.
    LOG(WARNING) << "[eyeo] Failed to add subscription, current number "
                 << "of subscriptions: " << current_state_.size();
    UpdatePreloadedSubscriptionProvider();
    return;
  }
  // Remove any subscription that already exists with the same URL
  bool subscription_existed =
      UninstallSubscriptionInternal(subscription->GetSourceUrl());
  // Add the new subscription
  current_state_.push_back(subscription);
  if (subscription_existed) {
    VLOG(1) << "[eyeo] Updated subscription " << subscription->GetSourceUrl()
            << ", current version " << subscription->GetCurrentVersion();
  } else {
    VLOG(1) << "[eyeo] Added subscription " << subscription->GetSourceUrl()
            << ", current number of subscriptions: " << current_state_.size();
  }
  UpdatePreloadedSubscriptionProvider();
  // Notify "observer"
  subscription_updated_callback_.Run(subscription->GetSourceUrl());
}

void FilteringConfigurationMaintainerImpl::PingAcceptableAds() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  downloader_->DoHeadRequest(
      AcceptableAdsUrl(),
      base::BindOnce(&FilteringConfigurationMaintainerImpl::OnHeadRequestDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FilteringConfigurationMaintainerImpl::OnHeadRequestDone(
    const std::string version) {
  if (version.empty()) {
    return;
  }
  persistent_metadata_->SetVersion(AcceptableAdsUrl(), version);
  persistent_metadata_->SetExpirationInterval(
      AcceptableAdsUrl(), kDefaultHeadRequestExpirationInterval);
}

void FilteringConfigurationMaintainerImpl::UninstallSubscription(
    const GURL& subscription_url) {
  DVLOG(1) << "[eyeo] Removing subscription " << subscription_url;
  if (!UninstallSubscriptionInternal(subscription_url)) {
    VLOG(1) << "[eyeo] Nothing to remove, subscription not installed "
            << subscription_url;
    return;
  }
  if (subscription_url != AcceptableAdsUrl()) {
    // Remove metadata associated with the subscription. Retain (forever)
    // metadata of the Acceptable Ads subscription even when it's no longer
    // installed, to allow continued HEAD-only pings for user counting purposes.
    persistent_metadata_->RemoveMetadata(subscription_url);
  }
  UpdatePreloadedSubscriptionProvider();
  VLOG(1) << "[eyeo] Removed subscription " << subscription_url;
}

bool FilteringConfigurationMaintainerImpl::UninstallSubscriptionInternal(
    const GURL& subscription_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  bool subscription_removed = false;
  auto it = base::ranges::find(current_state_, subscription_url,
                               &Subscription::GetSourceUrl);
  if (it != current_state_.end()) {
    storage_->RemoveSubscription(*it);
    current_state_.erase(it);
    subscription_removed = true;
  }

  auto ongoing_installation_it = base::ranges::find(
      ongoing_installations_, subscription_url, &Subscription::GetSourceUrl);
  if (ongoing_installation_it != ongoing_installations_.end()) {
    ongoing_installations_.erase(ongoing_installation_it);
    DVLOG(1) << "[eyeo] Canceling installation of subscription "
             << subscription_url;
    downloader_->CancelDownload(subscription_url);
    DVLOG(2) << "[eyeo] Canceled installation of subscription "
             << subscription_url;
    subscription_removed = true;
  }
  return subscription_removed;
}

void FilteringConfigurationMaintainerImpl::SetCustomFilters() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::vector<std::string> filters = configuration_->GetCustomFilters();
  base::ranges::transform(configuration_->GetAllowedDomains(),
                          std::back_inserter(filters),
                          &utils::CreateDomainAllowlistingFilter);
  for (const auto& filter : filters) {
    VLOG(1) << "[eyeo] Setting custom filter: " << filter;
  }
  if (filters.empty()) {
    custom_filters_.reset();
    return;
  }

  custom_filters_ = conversion_executor_->ConvertCustomFilters(filters);
}

void FilteringConfigurationMaintainerImpl::
    UpdatePreloadedSubscriptionProvider() {
  preloaded_subscription_provider_->UpdateSubscriptions(
      GetReadySubscriptions(), GetPendingSubscriptions());
}

std::vector<GURL> FilteringConfigurationMaintainerImpl::GetReadySubscriptions()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<GURL> result;
  result.reserve(current_state_.size());
  base::ranges::transform(current_state_, std::back_inserter(result),
                          &Subscription::GetSourceUrl);
  return result;
}

std::vector<GURL>
FilteringConfigurationMaintainerImpl::GetPendingSubscriptions() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<GURL> result;
  result.reserve(ongoing_installations_.size());
  base::ranges::transform(ongoing_installations_, std::back_inserter(result),
                          &Subscription::GetSourceUrl);
  return result;
}

}  // namespace adblock
