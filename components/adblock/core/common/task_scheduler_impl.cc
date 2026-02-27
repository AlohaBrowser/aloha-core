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

#include <algorithm>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"

namespace adblock {

// ALOHA https://app.clickup.com/t/86ewwdj1m: wall-clock based scheduling, see
// the header for the rationale.
TaskSchedulerImpl::TaskSchedulerImpl(base::TimeDelta check_interval,
                                     base::TimeDelta initial_delay,
                                     const base::Clock* clock)
    : check_interval_(check_interval),
      initial_delay_(initial_delay),
      clock_(clock ? clock : base::DefaultClock::GetInstance()) {}

TaskSchedulerImpl::~TaskSchedulerImpl() = default;

void TaskSchedulerImpl::StartSchedule(base::RepeatingClosure task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!timer_.IsRunning());
  task_ = std::move(task);
  VLOG(1) << "[eyeo] Starting task schedule";
  if (initial_delay_.is_zero()) {
    ExecuteTask();
  } else {
    VLOG(1) << "[eyeo] Initial task run scheduled for "
            << clock_->Now() + initial_delay_;
    ScheduleRunAfter(initial_delay_);  // ALOHA CU-86ewwdj1m
  }
}

void TaskSchedulerImpl::StopSchedule() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "[eyeo] Stopping task schedule";
  timer_.Stop();
}

void TaskSchedulerImpl::ExecuteTask() {
  VLOG(1) << "[eyeo] Executing task";
  task_.Run();
  VLOG(1) << "[eyeo] Task executed, next run scheduled for "
          << clock_->Now() + check_interval_;
  ScheduleRunAfter(check_interval_);  // ALOHA CU-86ewwdj1m
}

// ALOHA https://app.clickup.com/t/86ewwdj1m
void TaskSchedulerImpl::ScheduleRunAfter(base::TimeDelta delay) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduled_delay_ = delay;
  next_run_time_ = clock_->Now() + delay;
  timer_.Start(FROM_HERE, std::min(delay, kPollInterval),
               base::BindOnce(&TaskSchedulerImpl::OnPollTimer,
                              weak_ptr_factory_.GetWeakPtr()));
}

// ALOHA https://app.clickup.com/t/86ewwdj1m
void TaskSchedulerImpl::OnPollTimer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const base::TimeDelta remaining = next_run_time_ - clock_->Now();
  // Run when due. Also run if the wall clock jumped backwards past the moment
  // the run was scheduled (e.g. the user changed the date): the deadline is
  // then further away than the delay itself and would stall the task.
  if (remaining <= base::TimeDelta() || remaining > scheduled_delay_) {
    ExecuteTask();
    return;
  }
  timer_.Start(FROM_HERE, std::min(remaining, kPollInterval),
               base::BindOnce(&TaskSchedulerImpl::OnPollTimer,
                              weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace adblock
