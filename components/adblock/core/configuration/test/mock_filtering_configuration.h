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

#ifndef COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_MOCK_FILTERING_CONFIGURATION_H_
#define COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_MOCK_FILTERING_CONFIGURATION_H_

#include "base/observer_list.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockFilteringConfiguration : public NiceMock<FilteringConfiguration> {
 public:
  MockFilteringConfiguration();
  ~MockFilteringConfiguration() override;

  void AddObserver(FilteringConfiguration::Observer* observer) override;
  void RemoveObserver(FilteringConfiguration::Observer* observer) override;

  // The name must be unique across all created configurations.
  MOCK_METHOD(const std::string&, GetName, (), (const, override));

  // Enable or disable the entire configuration. A disabled configuration does
  // not contribute filters to classification and behaves as if it was not
  // installed.
  MOCK_METHOD(void, SetEnabled, (bool enabled), (override));
  MOCK_METHOD(bool, IsEnabled, (), (const, override));

  // Adding an existing filter list, or removing a non-existing filter list, are
  // NOPs and do not notify observers.
  MOCK_METHOD(void, AddFilterList, (const GURL& url), (override));
  MOCK_METHOD(void, RemoveFilterList, (const GURL& url), (override));
  MOCK_METHOD(std::vector<GURL>, GetFilterLists, (), (const, override));
  MOCK_METHOD(bool, IsFilterListPresent, (const GURL& url), (const, override));

  // Adding an existing allowed domain, or removing a non-existing allowed
  // domain, are NOPs and do not notify observers.
  MOCK_METHOD(void, AddAllowedDomain, (const std::string& domain), (override));
  MOCK_METHOD(void,
              RemoveAllowedDomain,
              (const std::string& domain),
              (override));
  MOCK_METHOD(std::vector<std::string>,
              GetAllowedDomains,
              (),
              (const, override));

  // Adding an existing custom filter, or removing a non-existing custom filter,
  // are NOPs and do not notify observers.
  MOCK_METHOD(void, AddCustomFilter, (const std::string& filter), (override));
  MOCK_METHOD(void,
              RemoveCustomFilter,
              (const std::string& filter),
              (override));
  MOCK_METHOD(std::vector<std::string>,
              GetCustomFilters,
              (),
              (const, override));

  base::ObserverList<FilteringConfiguration::Observer> observers_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONFIGURATION_TEST_MOCK_FILTERING_CONFIGURATION_H_
