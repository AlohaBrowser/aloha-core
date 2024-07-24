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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_IMPL_H_

#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/subscription_collection.h"

namespace adblock {

class SubscriptionCollectionImpl final : public SubscriptionCollection {
 public:
  explicit SubscriptionCollectionImpl(
      std::vector<scoped_refptr<InstalledSubscription>> current_state,
      const std::string& configuration_name);
  ~SubscriptionCollectionImpl() final;
  SubscriptionCollectionImpl(const SubscriptionCollectionImpl&);
  SubscriptionCollectionImpl(SubscriptionCollectionImpl&&);
  SubscriptionCollectionImpl& operator=(const SubscriptionCollectionImpl&);
  SubscriptionCollectionImpl& operator=(SubscriptionCollectionImpl&&);

  const std::string& GetFilteringConfigurationName() const final;

  absl::optional<GURL> FindBySubresourceFilter(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey,
      FilterCategory category) const final;

  absl::optional<GURL> FindByPopupFilter(
      const GURL& popup_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey,
      FilterCategory category) const final;

  absl::optional<GURL> FindByAllowFilter(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey) const final;

  absl::optional<GURL> FindBySpecialFilter(
      SpecialFilterType filter_type,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey) const final;

  InstalledSubscription::ContentFiltersData GetElementHideData(
      const GURL& frame_url,
      const std::vector<GURL>& frame_hierarchy,
      const SiteKey& sitekey) const final;
  InstalledSubscription::ContentFiltersData GetElementHideEmulationData(
      const GURL& frame_url) const final;
  base::Value::List GenerateSnippets(
      const GURL& frame_url,
      const std::vector<GURL>& frame_hierarchy) const final;
  std::set<std::string_view> GetCspInjections(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy) const final;
  std::set<std::string_view> GetRewriteFilters(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      FilterCategory category) const final;

  std::set<HeaderFilterData> GetHeaderFilters(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      FilterCategory category) const final;

 private:
  std::vector<scoped_refptr<InstalledSubscription>> subscriptions_;
  std::string configuration_name_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_COLLECTION_IMPL_H_
