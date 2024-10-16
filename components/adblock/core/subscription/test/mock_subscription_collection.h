
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_COLLECTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_COLLECTION_H_

#include "components/adblock/core/subscription/subscription_collection.h"

#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockSubscriptionCollection : public NiceMock<SubscriptionCollection> {
 public:
  MockSubscriptionCollection();
  ~MockSubscriptionCollection() override;
  MOCK_METHOD(const std::string&,
              GetFilteringConfigurationName,
              (),
              (const, override));
  MOCK_METHOD(absl::optional<GURL>,
              FindBySubresourceFilter,
              (const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               ContentType content_type,
               const SiteKey& sitekey,
               FilterCategory category),
              (const, override));
  MOCK_METHOD(absl::optional<GURL>,
              FindByPopupFilter,
              (const GURL& popup_url,
               const std::vector<GURL>& frame_hierarchy,
               const SiteKey& sitekey,
               FilterCategory category),
              (const, override));
  MOCK_METHOD(absl::optional<GURL>,
              FindByAllowFilter,
              (const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               ContentType content_type,
               const SiteKey& sitekey),
              (const, override));
  MOCK_METHOD(absl::optional<GURL>,
              FindBySpecialFilter,
              (SpecialFilterType filter_type,
               const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               const SiteKey& sitekey),
              (const, override));
  MOCK_METHOD(InstalledSubscription::ContentFiltersData,
              GetElementHideData,
              (const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               const SiteKey& sitekey),
              (const, override));
  MOCK_METHOD(InstalledSubscription::ContentFiltersData,
              GetElementHideEmulationData,
              (const GURL& frame_url),
              (const, override));
  MOCK_METHOD(base::Value::List,
              GenerateSnippets,
              (const GURL& frame_url, const std::vector<GURL>& frame_hierarchy),
              (const, override));
  MOCK_METHOD(std::set<std::string_view>,
              GetCspInjections,
              (const GURL& frame_url, const std::vector<GURL>& frame_hierarchy),
              (const, override));
  MOCK_METHOD(std::set<std::string_view>,
              GetRewriteFilters,
              (const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               FilterCategory blocking_filter_category),
              (const, override));
  MOCK_METHOD(std::set<HeaderFilterData>,
              GetHeaderFilters,
              (const GURL& frame_url,
               const std::vector<GURL>& frame_hierarchy,
               ContentType content_type,
               FilterCategory category),
              (const, override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_COLLECTION_H_
