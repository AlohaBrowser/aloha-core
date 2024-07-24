
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_INSTALLED_SUBSCRIPTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_INSTALLED_SUBSCRIPTION_H_

#include "components/adblock/core/subscription/installed_subscription.h"

#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockInstalledSubscription : public NiceMock<InstalledSubscription> {
 public:
  MockInstalledSubscription();
  MOCK_METHOD(GURL, GetSourceUrl, (), (override, const));
  MOCK_METHOD(std::string, GetTitle, (), (override, const));
  MOCK_METHOD(std::string, GetCurrentVersion, (), (override, const));
  MOCK_METHOD(InstallationState, GetInstallationState, (), (override, const));
  MOCK_METHOD(base::Time, GetInstallationTime, (), (override, const));
  MOCK_METHOD(base::TimeDelta, GetExpirationInterval, (), (override, const));
  MOCK_METHOD(bool,
              HasUrlFilter,
              (const GURL& url,
               const std::string& document_domain,
               ContentType type,
               const SiteKey& sitekey,
               FilterCategory category),
              (override, const));
  MOCK_METHOD(bool,
              HasPopupFilter,
              (const GURL& url,
               const std::string& document_domain,
               const SiteKey& sitekey,
               FilterCategory category),
              (override, const));
  MOCK_METHOD(bool,
              HasSpecialFilter,
              (SpecialFilterType type,
               const GURL& url,
               const std::string& document_domain,
               const SiteKey& sitekey),
              (override, const));
  MOCK_METHOD(void,
              FindCspFilters,
              (const GURL& url,
               const std::string& document_domain,
               FilterCategory category,
               std::set<std::string_view>& results),
              (override, const));
  MOCK_METHOD(std::set<std::string_view>,
              FindRewriteFilters,
              (const GURL& url,
               const std::string& document_domain,
               FilterCategory category),
              (override, const));
  MOCK_METHOD(void,
              FindHeaderFilters,
              (const GURL& url,
               ContentType type,
               const std::string& document_domain,
               FilterCategory category,
               std::set<HeaderFilterData>& results),
              (override, const));
  MOCK_METHOD(ContentFiltersData,
              GetElemhideData,
              (const GURL& url, bool domain_specific),
              (override, const));
  MOCK_METHOD(ContentFiltersData,
              GetElemhideEmulationData,
              (const GURL& url),
              (override, const));
  MOCK_METHOD(std::vector<Snippet>,
              MatchSnippets,
              (const std::string& document_domain),
              (override, const));
  MOCK_METHOD(void, MarkForPermanentRemoval, (), (override));

 protected:
  ~MockInstalledSubscription() override;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_INSTALLED_SUBSCRIPTION_H_
