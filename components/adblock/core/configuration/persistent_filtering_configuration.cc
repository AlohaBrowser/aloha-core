
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

#include "components/adblock/core/configuration/persistent_filtering_configuration.h"

#include <string>
#include <string_view>

#include "base/strings/string_util.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

#include "base/logging.h"

namespace adblock {
namespace {

constexpr auto kEnabledKey = std::string_view("enabled");
constexpr auto kDomainsKey = std::string_view("domains");
constexpr auto kCustomFiltersKey = std::string_view("filters");
constexpr auto kFilterListsKey = std::string_view("subscriptions");

base::Value::Dict ReadFromPrefs(PrefService* pref_service,
                                std::string_view configuration_name) {
  const auto& all_configurations =
      pref_service->GetValue(common::prefs::kConfigurationsPrefsPath).GetDict();
  const auto* this_config = all_configurations.FindDict(configuration_name);
  if (this_config) {
    return base::Value::Dict(this_config->Clone());
  }
  return base::Value::Dict();
}

void StoreToPrefs(const base::Value::Dict& configuration,
                  PrefService* pref_service,
                  std::string_view configuration_name) {
  // ScopedDictPrefUpdate requires an std::string for some reason:
  static std::string kConfigurationsPrefsPathString(
      common::prefs::kConfigurationsPrefsPath);
  ScopedDictPrefUpdate update(pref_service, kConfigurationsPrefsPathString);
  update.Get().Set(configuration_name, configuration.Clone());
}

void SetDefaultValuesIfNeeded(base::Value::Dict& configuration) {
  if (!configuration.FindBool(kEnabledKey)) {
    configuration.Set(kEnabledKey, true);
  }
  configuration.EnsureList(kDomainsKey);
  configuration.EnsureList(kCustomFiltersKey);
  configuration.EnsureList(kFilterListsKey);
}

bool AppendToList(base::Value::Dict& configuration,
                  std::string_view key,
                  const std::string& value) {
  DCHECK(configuration.FindList(key));  // see SetDefaultValuesIfNeeded().
  auto* list = configuration.FindList(key);
  if (base::ranges::find(*list, base::Value(value)) != list->end()) {
    // value already exists in the list.
    return false;
  }
  list->Append(value);
  return true;
}

bool RemoveFromList(base::Value::Dict& configuration,
                    std::string_view key,
                    const std::string& value) {
  DCHECK(configuration.FindList(key));  // see SetDefaultValuesIfNeeded().
  auto* list = configuration.FindList(key);
  auto it = base::ranges::find(*list, base::Value(value));
  if (it == list->end()) {
    // value was not on the list.
    return false;
  }
  list->erase(it);
  return true;
}

template <typename T>
std::vector<T> GetFromList(const base::Value::Dict& configuration,
                           std::string_view key) {
  DCHECK(configuration.FindList(key));  // see SetDefaultValuesIfNeeded().
  const auto* list = configuration.FindList(key);
  std::vector<T> result;
  for (const auto& value : *list) {
    if (value.is_string()) {
      result.emplace_back(value.GetString());
    }
  }
  return result;
}

}  // namespace

PersistentFilteringConfiguration::PersistentFilteringConfiguration(
    PrefService* pref_service,
    std::string name)
    : pref_service_(pref_service),
      name_(std::move(name)),
      dictionary_(ReadFromPrefs(pref_service_, name_)) {
  SetDefaultValuesIfNeeded(dictionary_);
  PersistToPrefs();
}

PersistentFilteringConfiguration::~PersistentFilteringConfiguration() = default;

void PersistentFilteringConfiguration::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}
void PersistentFilteringConfiguration::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

const std::string& PersistentFilteringConfiguration::GetName() const {
  return name_;
}

void PersistentFilteringConfiguration::SetEnabled(bool enabled) {
  if (IsEnabled() == enabled) {
    return;
  }
  dictionary_.Set(kEnabledKey, enabled);
  PersistToPrefs();
  NotifyEnabledStateChanged();
}

bool PersistentFilteringConfiguration::IsEnabled() const {
  const auto pref_value = dictionary_.FindBool(kEnabledKey);
  DCHECK(pref_value);
  return *pref_value;
}

void PersistentFilteringConfiguration::AddFilterList(const GURL& url) {
  if (AppendToList(dictionary_, kFilterListsKey, url.spec())) {
    PersistToPrefs();
    NotifyFilterListsChanged();
  }
}

void PersistentFilteringConfiguration::RemoveFilterList(const GURL& url) {
  if (RemoveFromList(dictionary_, kFilterListsKey, url.spec())) {
    PersistToPrefs();
    NotifyFilterListsChanged();
  }
}

std::vector<GURL> PersistentFilteringConfiguration::GetFilterLists() const {
  return GetFromList<GURL>(dictionary_, kFilterListsKey);
}

bool PersistentFilteringConfiguration::IsFilterListPresent(
    const GURL& url) const {
  return base::ranges::any_of(
      GetFilterLists(),
      [&](const GURL& filetr_list_url) { return filetr_list_url == url; });
}

void PersistentFilteringConfiguration::AddAllowedDomain(
    const std::string& domain) {
  if (AppendToList(dictionary_, kDomainsKey, domain)) {
    PersistToPrefs();
    NotifyAllowedDomainsChanged();
  }
}

void PersistentFilteringConfiguration::RemoveAllowedDomain(
    const std::string& domain) {
  if (RemoveFromList(dictionary_, kDomainsKey, domain)) {
    PersistToPrefs();
    NotifyAllowedDomainsChanged();
  }
}

std::vector<std::string> PersistentFilteringConfiguration::GetAllowedDomains()
    const {
  return GetFromList<std::string>(dictionary_, kDomainsKey);
}

void PersistentFilteringConfiguration::AddCustomFilter(
    const std::string& filter) {
  if (AppendToList(dictionary_, kCustomFiltersKey, filter)) {
    PersistToPrefs();
    NotifyCustomFiltersChanged();
  }
}

void PersistentFilteringConfiguration::RemoveCustomFilter(
    const std::string& filter) {
  if (RemoveFromList(dictionary_, kCustomFiltersKey, filter)) {
    PersistToPrefs();
    NotifyCustomFiltersChanged();
  }
}

std::vector<std::string> PersistentFilteringConfiguration::GetCustomFilters()
    const {
  return GetFromList<std::string>(dictionary_, kCustomFiltersKey);
}

void PersistentFilteringConfiguration::PersistToPrefs() {
  StoreToPrefs(dictionary_, pref_service_, name_);
}

void PersistentFilteringConfiguration::NotifyEnabledStateChanged() {
  for (auto& o : observers_) {
    o.OnEnabledStateChanged(this);
  }
}

void PersistentFilteringConfiguration::NotifyFilterListsChanged() {
  for (auto& o : observers_) {
    o.OnFilterListsChanged(this);
  }
}

void PersistentFilteringConfiguration::NotifyAllowedDomainsChanged() {
  for (auto& o : observers_) {
    o.OnAllowedDomainsChanged(this);
  }
}

void PersistentFilteringConfiguration::NotifyCustomFiltersChanged() {
  for (auto& o : observers_) {
    o.OnCustomFiltersChanged(this);
  }
}

// static
std::vector<std::unique_ptr<PersistentFilteringConfiguration>>
PersistentFilteringConfiguration::GetPersistedConfigurations(
    PrefService* pref_service) {
  std::vector<std::unique_ptr<PersistentFilteringConfiguration>> configs;
  const auto& all_configurations =
      pref_service->GetValue(common::prefs::kConfigurationsPrefsPath).GetDict();
  for (auto it = all_configurations.begin(); it != all_configurations.end();
       ++it) {
    configs.push_back(std::make_unique<PersistentFilteringConfiguration>(
        pref_service, (it->first)));
  }
  return configs;
}

// static
void PersistentFilteringConfiguration::RemovePersistedData(
    PrefService* pref_service,
    const std::string& name) {
  static std::string kConfigurationsPrefsPathString(
      common::prefs::kConfigurationsPrefsPath);
  ScopedDictPrefUpdate update(pref_service, kConfigurationsPrefsPathString);
  update.Get().Remove(name);
}

}  // namespace adblock
