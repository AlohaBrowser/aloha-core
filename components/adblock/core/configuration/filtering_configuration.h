
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

#ifndef COMPONENTS_ADBLOCK_CORE_CONFIGURATION_FILTERING_CONFIGURATION_H_
#define COMPONENTS_ADBLOCK_CORE_CONFIGURATION_FILTERING_CONFIGURATION_H_

#include <string>

#include "base/observer_list_types.h"
#include "url/gurl.h"

namespace adblock {

// A group of settings that control how to perform resource filtering.
//
// FilterConfigurations can be installed into a SubscriptionService.
// SubscriptionService interprets how to express each configuration in terms of
// installed subscriptions, and how to enact filtering.
//
// Each configuration is independent from others. If two FilteringConfigurations
// have filters that result in a conflicting classification decision,
// blocking a resource takes precedence over allowing a resource.
//
// Examples of multiple FilteringConfigurations could be:
// - one configuration to filter ads
// - another configuration to protect user privacy
// - another configuration to enforce parental control
// Each of these could be disabled/enabled or reconfigured individually, without
// affecting others.
class FilteringConfiguration {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnEnabledStateChanged(FilteringConfiguration* config) {}
    virtual void OnFilterListsChanged(FilteringConfiguration* config) {}
    virtual void OnAllowedDomainsChanged(FilteringConfiguration* config) {}
    virtual void OnCustomFiltersChanged(FilteringConfiguration* config) {}
  };

  virtual ~FilteringConfiguration() = default;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // The name must be unique across all created configurations.
  virtual const std::string& GetName() const = 0;

  // Enable or disable the entire configuration. A disabled configuration does
  // not contribute filters to classification and behaves as if it was not
  // installed.
  virtual void SetEnabled(bool enabled) = 0;
  virtual bool IsEnabled() const = 0;

  // Adding an existing filter list, or removing a non-existing filter list, are
  // NOPs and do not notify observers.
  virtual void AddFilterList(const GURL& url) = 0;
  virtual void RemoveFilterList(const GURL& url) = 0;
  virtual std::vector<GURL> GetFilterLists() const = 0;
  virtual bool IsFilterListPresent(const GURL& url) const = 0;

  // Adding an existing allowed domain, or removing a non-existing allowed
  // domain, are NOPs and do not notify observers.
  virtual void AddAllowedDomain(const std::string& domain) = 0;
  virtual void RemoveAllowedDomain(const std::string& domain) = 0;
  virtual std::vector<std::string> GetAllowedDomains() const = 0;

  // Adding an existing custom filter, or removing a non-existing custom filter,
  // are NOPs and do not notify observers.
  virtual void AddCustomFilter(const std::string& filter) = 0;
  virtual void RemoveCustomFilter(const std::string& filter) = 0;
  virtual std::vector<std::string> GetCustomFilters() const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONFIGURATION_FILTERING_CONFIGURATION_H_
