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

#ifndef COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_IMPL_H_

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/adblock/core/net/adblock_request_throttle.h"
#include "net/base/network_change_notifier.h"

namespace adblock {

class AdblockRequestThrottleImpl final
    : public AdblockRequestThrottle,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  AdblockRequestThrottleImpl();
  ~AdblockRequestThrottleImpl() final;

  void RunWhenRequestsAllowed(base::OnceClosure callback) final;

  void AllowRequestsAfter(base::TimeDelta delay) final;

  // NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType connection_type) final;

 private:
  bool AreRequestsAllowed() const;
  void MaybeRunDeferredCallbacks();

  base::OneShotTimer timer_;
  std::vector<base::OnceClosure> pending_callbacks_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_IMPL_H_
