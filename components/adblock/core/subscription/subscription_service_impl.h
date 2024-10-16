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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/filtering_configuration_maintainer.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/preloaded_subscription_provider.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/subscription_persistent_storage.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_service.h"

namespace adblock {

class SubscriptionServiceImpl final : public SubscriptionService,
                                      public FilteringConfiguration::Observer {
 public:
  // Used to notify this about updates to installed subscriptions.
  using SubscriptionUpdatedCallback =
      base::RepeatingCallback<void(const GURL& subscription_url)>;
  // Used to create FilteringConfigurationMaintainers for newly installed
  // FilteringConfigurations.
  using FilteringConfigurationMaintainerFactory =
      base::RepeatingCallback<std::unique_ptr<FilteringConfigurationMaintainer>(
          FilteringConfiguration* configuration,
          SubscriptionUpdatedCallback observer)>;
  using FilteringConfigurationCleaner =
      base::RepeatingCallback<void(FilteringConfiguration* configuration)>;
  explicit SubscriptionServiceImpl(
      PrefService* pref_service,
      FilteringConfigurationMaintainerFactory maintainer_factory,
      FilteringConfigurationCleaner configuration_cleaner);
  ~SubscriptionServiceImpl() final;

  // SubscriptionService:
  std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions(
      FilteringConfiguration* configuration) const final;
  void InstallFilteringConfiguration(
      std::unique_ptr<FilteringConfiguration> configuration) final;
  void UninstallFilteringConfiguration(
      std::string_view configuration_name) final;
  std::vector<FilteringConfiguration*> GetInstalledFilteringConfigurations()
      final;
  FilteringConfiguration* GetFilteringConfiguration(
      std::string_view configuration_name) const final;
  Snapshot GetCurrentSnapshot() const final;
  void SetAutoInstallEnabled(bool enabled) final;
  bool IsAutoInstallEnabled() const final;
  void AddObserver(SubscriptionObserver*) final;
  void RemoveObserver(SubscriptionObserver*) final;

  // FilteringConfiguration::Observer:
  void OnEnabledStateChanged(FilteringConfiguration* config) final;

 private:
  void OnSubscriptionUpdated(const GURL& subscription_url);
  std::unique_ptr<FilteringConfigurationMaintainer> MakeMaintainer(
      FilteringConfiguration* configuration);

  SEQUENCE_CHECKER(sequence_checker_);
  const raw_ptr<PrefService> pref_service_;
  FilteringConfigurationMaintainerFactory maintainer_factory_;
  FilteringConfigurationCleaner configuration_cleaner_;
  using MaintainersCollection =
      std::map<std::unique_ptr<FilteringConfiguration>,
               std::unique_ptr<FilteringConfigurationMaintainer>>;
  MaintainersCollection maintainers_;
  base::ObserverList<SubscriptionObserver> observers_;
  base::WeakPtrFactory<SubscriptionServiceImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_
