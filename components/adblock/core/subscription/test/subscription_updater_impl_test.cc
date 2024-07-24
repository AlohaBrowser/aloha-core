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

#include "components/adblock/core/subscription/subscription_updater_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {
constexpr auto kInitialDelay = base::Minutes(1);
constexpr auto kCheckInterval = base::Hours(1);
}  // namespace

class AdblockSubscriptionUpdaterImplTest : public testing::Test {
 public:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::MockRepeatingClosure run_update_check_;
  SubscriptionUpdaterImpl updater_{kInitialDelay, kCheckInterval};
};

TEST_F(AdblockSubscriptionUpdaterImplTest, UpdatesRanContinuously) {
  updater_.StartSchedule(run_update_check_.Get());
  // Schedule will run indefinitely after starting, with the first check
  // performed after initial delay and following checks after normal check
  // interval.
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay);
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       UpdatesNotRanAfterStoppingImmediately) {
  updater_.StartSchedule(run_update_check_.Get());
  // Stop immediately.
  updater_.StopSchedule();
  EXPECT_CALL(run_update_check_, Run()).Times(0);
  task_environment_.FastForwardBy(kInitialDelay);
  task_environment_.FastForwardBy(kCheckInterval);
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockSubscriptionUpdaterImplTest, UpdatesNotRanAfterStopping) {
  updater_.StartSchedule(run_update_check_.Get());
  // Let the checks run for some time.
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay);
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);

  // Stop now.
  updater_.StopSchedule();
  EXPECT_CALL(run_update_check_, Run()).Times(0);
  task_environment_.FastForwardBy(kCheckInterval);
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockSubscriptionUpdaterImplTest, Restarting) {
  updater_.StartSchedule(run_update_check_.Get());
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay);
  updater_.StopSchedule();

  updater_.StartSchedule(run_update_check_.Get());
  // After restarting, the schedule starts from initial delay.
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kInitialDelay);
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
  EXPECT_CALL(run_update_check_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
}

}  // namespace adblock
