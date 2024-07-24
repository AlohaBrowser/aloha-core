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

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/time/time.h"

namespace adblock {

SubscriptionUpdaterImpl::SubscriptionUpdaterImpl(base::TimeDelta initial_delay,
                                                 base::TimeDelta check_interval)
    : initial_delay_(initial_delay), check_interval_(check_interval) {}

SubscriptionUpdaterImpl::~SubscriptionUpdaterImpl() = default;

void SubscriptionUpdaterImpl::StartSchedule(
    base::RepeatingClosure run_update_check) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!timer_.IsRunning());
  run_update_check_ = std::move(run_update_check);
  VLOG(1) << "[eyeo] Starting update schedule, first check scheduled for "
          << base::Time::Now() + initial_delay_;
  timer_.Start(FROM_HERE, initial_delay_,
               base::BindOnce(&SubscriptionUpdaterImpl::RunUpdateCheck,
                              weak_ptr_factory_.GetWeakPtr()));
}

void SubscriptionUpdaterImpl::StopSchedule() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "[eyeo] Stopping update schedule";
  timer_.Stop();
}

void SubscriptionUpdaterImpl::RunUpdateCheck() {
  VLOG(1) << "[eyeo] Running subscription update check";
  run_update_check_.Run();
  VLOG(1)
      << "[eyeo] Subscription update check completed, next one scheduled for "
      << base::Time::Now() + check_interval_;
  timer_.Start(FROM_HERE, check_interval_,
               base::BindOnce(&SubscriptionUpdaterImpl::RunUpdateCheck,
                              weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace adblock
