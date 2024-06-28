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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/configuration/test/fake_filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"

#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockSubscriptionService : public NiceMock<SubscriptionService> {
 public:
  MockSubscriptionService();
  ~MockSubscriptionService() override;
  MOCK_METHOD(std::vector<scoped_refptr<Subscription>>,
              GetCurrentSubscriptions,
              (FilteringConfiguration*),
              (override, const));
  MOCK_METHOD(Snapshot, GetCurrentSnapshot, (), (override, const));
  MOCK_METHOD(void,
              InstallFilteringConfiguration,
              (std::unique_ptr<FilteringConfiguration> configuration),
              (override));
  MOCK_METHOD(void,
              UninstallFilteringConfiguration,
              (std::string_view configuration_name),
              (override));
  MOCK_METHOD(std::vector<FilteringConfiguration*>,
              GetInstalledFilteringConfigurations,
              (),
              (override));
  MOCK_METHOD(FilteringConfiguration*,
              GetFilteringConfiguration,
              (std::string_view configuration_name),
              (override, const));
  void AddObserver(SubscriptionObserver* observer) final;
  void RemoveObserver(SubscriptionObserver* observer) final;

  void WillRequireFiltering(bool filtering_required) {
    filtering_configuration_.is_enabled = filtering_required;
    EXPECT_CALL(*this, GetInstalledFilteringConfigurations())
        .WillRepeatedly(testing::Return(
            std::vector<FilteringConfiguration*>{&filtering_configuration_}));
  }

  raw_ptr<SubscriptionObserver> observer_ = nullptr;
  FakeFilteringConfiguration filtering_configuration_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_
