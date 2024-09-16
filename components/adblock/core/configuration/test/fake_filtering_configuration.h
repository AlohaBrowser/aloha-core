
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

#ifndef COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_FAKE_FILTERING_CONFIGURATION_H_
#define COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_FAKE_FILTERING_CONFIGURATION_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "components/adblock/core/configuration/filtering_configuration.h"

namespace adblock {

class FakeFilteringConfiguration : public FilteringConfiguration {
 public:
  FakeFilteringConfiguration();
  explicit FakeFilteringConfiguration(std::string name);
  ~FakeFilteringConfiguration() override;

  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  const std::string& GetName() const override;

  void SetEnabled(bool enabled) override;
  bool IsEnabled() const override;

  void AddFilterList(const GURL& url) override;
  void RemoveFilterList(const GURL& url) override;
  std::vector<GURL> GetFilterLists() const override;
  bool IsFilterListPresent(const GURL& url) const override;

  void AddAllowedDomain(const std::string& domain) override;
  void RemoveAllowedDomain(const std::string& domain) override;
  std::vector<std::string> GetAllowedDomains() const override;

  void AddCustomFilter(const std::string& filter) override;
  void RemoveCustomFilter(const std::string& filter) override;
  std::vector<std::string> GetCustomFilters() const override;

  raw_ptr<Observer> observer = nullptr;
  std::string name;
  bool is_enabled = true;
  std::vector<GURL> filter_lists;
  std::vector<std::string> allowed_domains;
  std::vector<std::string> custom_filters;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_FAKE_FILTERING_CONFIGURATION_H_
