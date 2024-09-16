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

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/time/time.h"

namespace adblock {

TaskSchedulerImpl::TaskSchedulerImpl(base::TimeDelta initial_delay,
                                     base::TimeDelta check_interval)
    : initial_delay_(initial_delay), check_interval_(check_interval) {}

TaskSchedulerImpl::~TaskSchedulerImpl() = default;

void TaskSchedulerImpl::StartSchedule(base::RepeatingClosure task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!timer_.IsRunning());
  task_ = std::move(task);
  VLOG(1) << "[eyeo] Starting task schedule, first run scheduled for "
          << base::Time::Now() + initial_delay_;
  timer_.Start(FROM_HERE, initial_delay_,
               base::BindOnce(&TaskSchedulerImpl::ExecuteTask,
                              weak_ptr_factory_.GetWeakPtr()));
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
          << base::Time::Now() + check_interval_;
  timer_.Start(FROM_HERE, check_interval_,
               base::BindOnce(&TaskSchedulerImpl::ExecuteTask,
                              weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace adblock
