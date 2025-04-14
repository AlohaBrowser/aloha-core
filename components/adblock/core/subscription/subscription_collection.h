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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_H_

#include <set>
#include <string_view>
#include <vector>

#include "absl/types/optional.h"
#include "base/containers/span.h"
#include "base/values.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/common/header_filter_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "url/gurl.h"

namespace adblock {

// Represents a collection of all currently active Subscriptions and allows
// bulk queries to be made towards all of them.
// Represents a snapshot of a state of the browser.
// Cheap to create and copy, non-mutable and thread-safe.
class SubscriptionCollection {
 public:
  virtual ~SubscriptionCollection() = default;

  // Name of the FilteringConfiguration this collection represents
  virtual const std::string& GetFilteringConfigurationName() const = 0;

  virtual absl::optional<GURL> FindBySubresourceFilter(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey,
      FilterCategory category) const = 0;
  virtual absl::optional<GURL> FindByPopupFilter(
      const GURL& popup_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey,
      FilterCategory category) const = 0;
  virtual absl::optional<GURL> FindByAllowFilter(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey) const = 0;
  virtual absl::optional<GURL> FindBySpecialFilter(
      SpecialFilterType filter_type,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey) const = 0;

  virtual InstalledSubscription::ContentFiltersData GetElementHideData(
      const GURL& frame_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey) const = 0;
  virtual InstalledSubscription::ContentFiltersData GetElementHideEmulationData(
      const GURL& frame_url) const = 0;

  virtual base::Value::List GenerateSnippets(
      const GURL& frame_url,
      const std::vector<GURL>& frame_hierarchy) const = 0;

  virtual std::set<std::string_view> GetCspInjections(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy) const = 0;

  virtual std::set<std::string_view> GetRewriteFilters(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      FilterCategory category) const = 0;

  virtual std::set<HeaderFilterData> GetHeaderFilters(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      FilterCategory category) const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_H_
