
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_H_

#include "testing/gmock/include/gmock/gmock.h"

#include "components/adblock/core/subscription/subscription.h"

using testing::NiceMock;

namespace adblock {

class MockSubscription : public NiceMock<Subscription> {
 public:
  MockSubscription();
  MOCK_METHOD(GURL, GetSourceUrl, (), (override, const));
  MOCK_METHOD(std::string, GetTitle, (), (override, const));
  MOCK_METHOD(std::string, GetCurrentVersion, (), (override, const));
  MOCK_METHOD(InstallationState, GetInstallationState, (), (override, const));
  MOCK_METHOD(base::Time, GetInstallationTime, (), (override, const));
  MOCK_METHOD(base::TimeDelta, GetExpirationInterval, (), (override, const));

 protected:
  ~MockSubscription() override;
};

scoped_refptr<MockSubscription> MakeMockSubscription(
    GURL url,
    Subscription::InstallationState state =
        Subscription::InstallationState::Installed);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_H_
