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

#include "components/adblock/core/net/adblock_request_throttle_impl.h"

#include <memory>
#include <string_view>

#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "net/base/mock_network_change_notifier.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace adblock {

class AdblockRequestThrottleImplTest : public testing::Test {
 public:
  AdblockRequestThrottleImplTest() {
    request_throttle_.AllowRequestsAfter(kInitialDelay);
  }

  void SetOffline() {
    network_change_notifier_->SetConnectionTypeAndNotifyObservers(
        net::NetworkChangeNotifier::CONNECTION_NONE);
  }

  void SetOnline() {
    network_change_notifier_->SetConnectionTypeAndNotifyObservers(
        net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  }

  const base::TimeDelta kInitialDelay = base::Seconds(30);
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<net::test::MockNetworkChangeNotifier>
      network_change_notifier_ = net::test::MockNetworkChangeNotifier::Create();
  AdblockRequestThrottleImpl request_throttle_;
};

TEST_F(AdblockRequestThrottleImplTest, CallbackDelayedUntilRequestsAllowed) {
  base::MockOnceClosure callback;
  // kInitialDelay is 30 seconds, so the callback should not be run immediately.
  EXPECT_CALL(callback, Run()).Times(0);
  request_throttle_.RunWhenRequestsAllowed(callback.Get());

  // Advance time by half od kInitialDelay, the callback should still not be
  // run.
  task_environment_.FastForwardBy(kInitialDelay / 2);

  // Advance time by the remaining half of kInitialDelay, the callback should be
  // run now.
  EXPECT_CALL(callback, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay / 2);
}

TEST_F(AdblockRequestThrottleImplTest, CheckDelayedIfOffline) {
  SetOffline();
  base::MockOnceClosure callback;
  // The callback should not be run if the browser is offline.
  EXPECT_CALL(callback, Run()).Times(0);
  request_throttle_.RunWhenRequestsAllowed(callback.Get());

  // Advance time by kInitialDelay, the callback should still not be run.
  task_environment_.FastForwardBy(kInitialDelay);

  // Set the browser online, the callback should be run now.
  EXPECT_CALL(callback, Run()).Times(1);
  SetOnline();
}

TEST_F(AdblockRequestThrottleImplTest,
       GoingOnlineBeforeInitialDelayDoesNotTriggerPendingCallbacks) {
  SetOffline();
  base::MockOnceClosure callback;
  // The callback should not be run if the browser is offline.
  EXPECT_CALL(callback, Run()).Times(0);
  request_throttle_.RunWhenRequestsAllowed(callback.Get());

  // Advance time by half of kInitialDelay, the callback should still not be
  // run.
  task_environment_.FastForwardBy(kInitialDelay / 2);

  // Set the browser online, the callback should still not be run.
  SetOnline();

  // Advance time by the remaining half of kInitialDelay, the callback should be
  // run now.
  EXPECT_CALL(callback, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay / 2);
}

TEST_F(AdblockRequestThrottleImplTest,
       CallbackRunImmediatelyIfRequestsAllowed) {
  base::MockOnceClosure callback;
  // AllowRequestsAfter(base::Seconds(0)) should allow the callback to run
  // immediately.
  EXPECT_CALL(callback, Run()).Times(1);
  request_throttle_.AllowRequestsAfter(base::Seconds(0));
  request_throttle_.RunWhenRequestsAllowed(callback.Get());
}

TEST_F(AdblockRequestThrottleImplTest, PendingCallbacksRunAfterOverride) {
  base::MockOnceClosure callback1;
  base::MockOnceClosure callback2;
  // RunWhenRequestsAllowed() should queue the callbacks.
  EXPECT_CALL(callback1, Run()).Times(0);
  EXPECT_CALL(callback2, Run()).Times(0);
  request_throttle_.RunWhenRequestsAllowed(callback1.Get());
  request_throttle_.RunWhenRequestsAllowed(callback2.Get());

  // AllowRequestsAfter(base::Seconds(0)) should run all pending callbacks.
  EXPECT_CALL(callback1, Run()).Times(1);
  EXPECT_CALL(callback2, Run()).Times(1);
  request_throttle_.AllowRequestsAfter(base::Seconds(0));
}

}  // namespace adblock
