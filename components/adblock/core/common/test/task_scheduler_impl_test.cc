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

#include "components/adblock/core/common/task_scheduler_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {
constexpr auto kCheckInterval = base::Hours(1);
}  // namespace

class AdblockTaskSchedulerImplTest : public testing::Test {
 public:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::MockRepeatingClosure task_;
  TaskSchedulerImpl task_scheduler_{kCheckInterval};
};

TEST_F(AdblockTaskSchedulerImplTest, TaskRanContinuously) {
  // Schedule will run indefinitely after starting, with the first check
  // happening immediately and subsequent checks scheduled with check interval.
  EXPECT_CALL(task_, Run()).Times(1);
  task_scheduler_.StartSchedule(task_.Get());
  EXPECT_CALL(task_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
  EXPECT_CALL(task_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockTaskSchedulerImplTest, TaskNotRanAfterStopping) {
  EXPECT_CALL(task_, Run()).Times(1);
  task_scheduler_.StartSchedule(task_.Get());

  // Let the checks run for some time.
  EXPECT_CALL(task_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);

  // Stop now.
  task_scheduler_.StopSchedule();
  EXPECT_CALL(task_, Run()).Times(0);
  task_environment_.FastForwardBy(kCheckInterval);
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockTaskSchedulerImplTest, Restarting) {
  EXPECT_CALL(task_, Run()).Times(1);
  task_scheduler_.StartSchedule(task_.Get());
  task_scheduler_.StopSchedule();

  // After restarting, the schedule starts immediately again.
  EXPECT_CALL(task_, Run()).Times(1);
  task_scheduler_.StartSchedule(task_.Get());
  EXPECT_CALL(task_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
  EXPECT_CALL(task_, Run()).Times(1);
  task_environment_.FastForwardBy(kCheckInterval);
}

}  // namespace adblock
