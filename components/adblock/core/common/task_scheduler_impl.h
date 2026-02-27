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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_TASK_SCHEDULER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_TASK_SCHEDULER_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/adblock/core/common/task_scheduler.h"

namespace adblock {

// ALOHA https://app.clickup.com/t/86ewwdj1m
// Runs |task| once per |check_interval| of *wall-clock* time. base::Timer
// delays are measured in TimeTicks (CLOCK_MONOTONIC), which on Android stands
// still while the device is suspended, so a plain one-hour timer fires only
// after one hour of device uptime - on an idle phone that can be days of real
// time, while filter list expiration is checked against wall-clock time.
// The scheduler therefore keeps the next run time as base::Time and polls it
// with a short timer: after the device wakes up, the first overdue poll runs
// the task right away.
class TaskSchedulerImpl final : public TaskScheduler {
 public:
  // Granularity with which wall-clock time is polled.
  static constexpr base::TimeDelta kPollInterval = base::Minutes(1);

  // |clock| provides wall-clock time; it must outlive the scheduler.
  explicit TaskSchedulerImpl(base::TimeDelta check_interval,
                             base::TimeDelta initial_delay = base::TimeDelta(),
                             const base::Clock* clock = nullptr);
  ~TaskSchedulerImpl() final;
  void StartSchedule(base::RepeatingClosure run_update_check) final;
  void StopSchedule() final;

 private:
  void ExecuteTask();
  // ALOHA https://app.clickup.com/t/86ewwdj1m
  void ScheduleRunAfter(base::TimeDelta delay);
  void OnPollTimer();

  SEQUENCE_CHECKER(sequence_checker_);
  base::RepeatingClosure task_;
  const base::TimeDelta check_interval_;
  const base::TimeDelta initial_delay_;
  // ALOHA https://app.clickup.com/t/86ewwdj1m
  const raw_ptr<const base::Clock> clock_;
  // Wall-clock time of the next task run and the delay it was scheduled with.
  base::Time next_run_time_;
  base::TimeDelta scheduled_delay_;
  base::OneShotTimer timer_;
  base::WeakPtrFactory<TaskSchedulerImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_TASK_SCHEDULER_IMPL_H_
