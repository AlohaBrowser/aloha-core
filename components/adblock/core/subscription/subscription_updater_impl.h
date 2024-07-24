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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_UPDATER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_UPDATER_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "components/adblock/core/subscription/subscription_updater.h"

namespace adblock {

class SubscriptionUpdaterImpl final : public SubscriptionUpdater {
 public:
  SubscriptionUpdaterImpl(base::TimeDelta initial_delay,
                          base::TimeDelta check_interval);
  ~SubscriptionUpdaterImpl() final;
  void StartSchedule(base::RepeatingClosure run_update_check) final;
  void StopSchedule() final;

 private:
  void RunUpdateCheck();

  SEQUENCE_CHECKER(sequence_checker_);
  base::RepeatingClosure run_update_check_;
  const base::TimeDelta initial_delay_;
  const base::TimeDelta check_interval_;
  base::OneShotTimer timer_;
  base::WeakPtrFactory<SubscriptionUpdaterImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_UPDATER_IMPL_H_
