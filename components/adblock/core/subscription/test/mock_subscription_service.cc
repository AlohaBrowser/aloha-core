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

#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "gtest/gtest.h"

namespace adblock {

MockSubscriptionService::MockSubscriptionService() = default;
MockSubscriptionService::~MockSubscriptionService() = default;

void MockSubscriptionService::AddObserver(SubscriptionObserver* observer) {
  ASSERT_FALSE(observer_) << "Adding observer twice";
  observer_ = observer;
}

void MockSubscriptionService::RemoveObserver(SubscriptionObserver* observer) {
  ASSERT_EQ(observer_, observer) << "Removing unknown observer";
  observer_ = nullptr;
}

}  // namespace adblock
