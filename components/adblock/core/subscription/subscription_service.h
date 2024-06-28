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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list_types.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace adblock {

// Maintains a state of available Subscriptions needed for all installed
// FilteringConfigurations.
class SubscriptionService : public KeyedService {
 public:
  using Snapshot = std::vector<std::unique_ptr<SubscriptionCollection>>;
  class SubscriptionObserver : public base::CheckedObserver {
   public:
    // Called only on successful installation or update of a subscription.
    // TODO(mpawlowski) add error reporting.
    virtual void OnSubscriptionInstalled(const GURL& subscription_url) {}
    // Called on installation of new filtering configuration
    virtual void OnFilteringConfigurationInstalled(
        FilteringConfiguration* config) {}
    virtual void OnFilteringConfigurationUninstalled(
        std::string_view config_name) {}
  };
  // Returns currently available subscriptions installed for |configuration|.
  // Includes subscriptions that are still being downloaded.
  virtual std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions(
      FilteringConfiguration* configuration) const = 0;
  // Subscriptions and filters demanded by |configuration| will be installed and
  // will become part of future Snapshots. SubscriptionService will maintain
  // subscriptions required by the configuration, download and remove filter
  // lists as needed.
  virtual void InstallFilteringConfiguration(
      std::unique_ptr<FilteringConfiguration> configuration) = 0;
  // Removes configuration from the list of known configurations and reset its
  // all persistent data. Use it only when configuration is no longer needed,
  // otherwise prefer to disable configuration via FilteringConfiguration API.
  // IMPORTANT: After calling this method any pointer pointing to uninstalled
  // configuration becomes invalid.
  virtual void UninstallFilteringConfiguration(
      std::string_view configuration_name) = 0;
  // Returns a list of FilteringConfigurations previously installed via
  // InstallFilteringConfiguration.
  virtual std::vector<FilteringConfiguration*>
  GetInstalledFilteringConfigurations() = 0;
  // Gets a filtering configuration by name.
  virtual FilteringConfiguration* GetFilteringConfiguration(
      std::string_view configuration_name) const = 0;
  // Returns a snapshot of subscriptions as present at the time of calling the
  // function that can be used to query filters.
  // The result may be passed between threads, even called
  // concurrently, and future changes to the installed subscriptions will not
  // impact it.
  virtual Snapshot GetCurrentSnapshot() const = 0;

  virtual void AddObserver(SubscriptionObserver*) = 0;
  virtual void RemoveObserver(SubscriptionObserver*) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_
