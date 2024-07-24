
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

#include "components/adblock/core/configuration/test/fake_filtering_configuration.h"

#include "base/ranges/algorithm.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

FakeFilteringConfiguration::FakeFilteringConfiguration() : name("testing") {}

FakeFilteringConfiguration::FakeFilteringConfiguration(std::string name)
    : name(name) {}

FakeFilteringConfiguration::~FakeFilteringConfiguration() {
  EXPECT_FALSE(observer) << "Observer was not removed";
}

void FakeFilteringConfiguration::AddObserver(Observer* obs) {
  EXPECT_FALSE(observer) << "Observer was already added";
  observer = obs;
}

void FakeFilteringConfiguration::RemoveObserver(Observer* obs) {
  EXPECT_EQ(observer, obs) << "This fake works with just a single observer";
  observer = nullptr;
}

const std::string& FakeFilteringConfiguration::GetName() const {
  return name;
}

void FakeFilteringConfiguration::SetEnabled(bool enabled) {
  is_enabled = enabled;
  if (observer) {
    observer->OnEnabledStateChanged(this);
  }
}

bool FakeFilteringConfiguration::IsEnabled() const {
  return is_enabled;
}

void FakeFilteringConfiguration::AddFilterList(const GURL& url) {
  filter_lists.push_back(url);
  if (observer) {
    observer->OnFilterListsChanged(this);
  }
}

void FakeFilteringConfiguration::RemoveFilterList(const GURL& url) {
  filter_lists.erase(base::ranges::remove(filter_lists, url),
                     filter_lists.end());
  if (observer) {
    observer->OnFilterListsChanged(this);
  }
}

std::vector<GURL> FakeFilteringConfiguration::GetFilterLists() const {
  return filter_lists;
}

bool FakeFilteringConfiguration::IsFilterListPresent(const GURL& url) const {
  return base::ranges::any_of(
      GetFilterLists(),
      [&](const GURL& filetr_list_url) { return filetr_list_url == url; });
}

void FakeFilteringConfiguration::AddAllowedDomain(const std::string& domain) {
  allowed_domains.push_back(domain);
  if (observer) {
    observer->OnAllowedDomainsChanged(this);
  }
}

void FakeFilteringConfiguration::RemoveAllowedDomain(
    const std::string& domain) {
  allowed_domains.erase(base::ranges::remove(allowed_domains, domain),
                        allowed_domains.end());
  if (observer) {
    observer->OnAllowedDomainsChanged(this);
  }
}

std::vector<std::string> FakeFilteringConfiguration::GetAllowedDomains() const {
  return allowed_domains;
}

void FakeFilteringConfiguration::AddCustomFilter(const std::string& filter) {
  custom_filters.push_back(filter);
  if (observer) {
    observer->OnCustomFiltersChanged(this);
  }
}

void FakeFilteringConfiguration::RemoveCustomFilter(const std::string& filter) {
  custom_filters.erase(base::ranges::remove(custom_filters, filter),
                       custom_filters.end());
  if (observer) {
    observer->OnCustomFiltersChanged(this);
  }
}

std::vector<std::string> FakeFilteringConfiguration::GetCustomFilters() const {
  return custom_filters;
}

}  // namespace adblock
