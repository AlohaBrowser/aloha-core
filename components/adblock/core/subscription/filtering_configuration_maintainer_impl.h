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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_IMPL_H_

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/conversion_executors.h"
#include "components/adblock/core/subscription/filtering_configuration_maintainer.h"
#include "components/adblock/core/subscription/preloaded_subscription_provider.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/subscription_persistent_storage.h"
#include "components/adblock/core/subscription/subscription_updater.h"

namespace adblock {

class FilteringConfigurationMaintainerImpl
    : public FilteringConfigurationMaintainer,
      public FilteringConfiguration::Observer {
 public:
  using SubscriptionUpdatedCallback =
      base::RepeatingCallback<void(const GURL&)>;
  FilteringConfigurationMaintainerImpl(
      FilteringConfiguration* configuration,
      std::unique_ptr<SubscriptionPersistentStorage> storage,
      std::unique_ptr<SubscriptionDownloader> downloader,
      std::unique_ptr<PreloadedSubscriptionProvider>
          preloaded_subscription_provider,
      std::unique_ptr<SubscriptionUpdater> updater,
      ConversionExecutors* conversion_executor,
      SubscriptionPersistentMetadata* persistent_metadata,
      SubscriptionUpdatedCallback subscription_updated_callback);
  ~FilteringConfigurationMaintainerImpl() override;

  std::unique_ptr<SubscriptionCollection> GetSubscriptionCollection()
      const override;

  std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions()
      const override;

  // FilteringConfiguration::Observer:
  void OnFilterListsChanged(FilteringConfiguration* config) final;
  void OnAllowedDomainsChanged(FilteringConfiguration* config) final;
  void OnCustomFiltersChanged(FilteringConfiguration* config) final;

  void InitializeStorage();

 private:
  enum class StorageStatus {
    Initialized,
    Uninitialized,
  };
  class OngoingInstallation;
  bool IsInitialized() const;
  void InstallMissingSubscriptions();
  void RemoveUnneededSubscriptions();
  void StorageInitialized(
      std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions);
  void RemoveDuplicateSubscriptions();
  void RunUpdateCheck();
  void DownloadAndInstallSubscription(const GURL& subscription_url);
  void OnSubscriptionDataAvailable(
      scoped_refptr<OngoingInstallation> ongoing_installation,
      std::unique_ptr<FlatbufferData> raw_data);
  void SubscriptionAddedToStorage(
      scoped_refptr<OngoingInstallation> ongoing_installation,
      scoped_refptr<InstalledSubscription> subscription);
  void PingAcceptableAds();
  void OnHeadRequestDone(const std::string version);
  void UninstallSubscription(const GURL& subscription_url);
  bool UninstallSubscriptionInternal(const GURL& subscription_url);
  void SetCustomFilters();
  void UpdatePreloadedSubscriptionProvider();
  std::vector<GURL> GetReadySubscriptions() const;
  std::vector<GURL> GetPendingSubscriptions() const;

  SEQUENCE_CHECKER(sequence_checker_);
  StorageStatus status_ = StorageStatus::Uninitialized;
  raw_ptr<FilteringConfiguration> configuration_;
  std::unique_ptr<SubscriptionPersistentStorage> storage_;
  std::unique_ptr<SubscriptionDownloader> downloader_;
  std::unique_ptr<PreloadedSubscriptionProvider>
      preloaded_subscription_provider_;
  std::unique_ptr<SubscriptionUpdater> updater_;
  raw_ptr<ConversionExecutors> conversion_executor_;
  // TODO(mpawlowski): Should not need to update metadata after DPD-1154, when
  // HEAD requests are removed. Move all use of SubscriptionPersistentMetadata
  // into SubscriptionPersistentStorage.
  raw_ptr<SubscriptionPersistentMetadata> persistent_metadata_;
  SubscriptionUpdatedCallback subscription_updated_callback_;
  std::set<scoped_refptr<OngoingInstallation>> ongoing_installations_;
  std::vector<scoped_refptr<InstalledSubscription>> current_state_;
  scoped_refptr<InstalledSubscription> custom_filters_;
  base::WeakPtrFactory<FilteringConfigurationMaintainerImpl> weak_ptr_factory_{
      this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_IMPL_H_
