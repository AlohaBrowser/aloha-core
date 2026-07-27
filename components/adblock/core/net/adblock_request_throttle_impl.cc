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

namespace adblock {

AdblockRequestThrottleImpl::AdblockRequestThrottleImpl() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

AdblockRequestThrottleImpl::~AdblockRequestThrottleImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void AdblockRequestThrottleImpl::RunWhenRequestsAllowed(
    base::OnceClosure callback) {
  if (!AreRequestsAllowed()) {
    pending_callbacks_.push_back(std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void AdblockRequestThrottleImpl::AllowRequestsAfter(base::TimeDelta delay) {
  timer_.Stop();
  if (delay.is_zero()) {
    MaybeRunDeferredCallbacks();
    return;
  }
  timer_.Start(FROM_HERE, delay,
               base::BindRepeating(
                   &AdblockRequestThrottleImpl::MaybeRunDeferredCallbacks,
                   base::Unretained(this)));
}

void AdblockRequestThrottleImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  MaybeRunDeferredCallbacks();
}

bool AdblockRequestThrottleImpl::AreRequestsAllowed() const {
  // We must be online and past the initial delay.
  return !net::NetworkChangeNotifier::IsOffline() && !timer_.IsRunning();
}

void AdblockRequestThrottleImpl::MaybeRunDeferredCallbacks() {
  if (!AreRequestsAllowed()) {
    return;
  }
  for (auto& callback : pending_callbacks_) {
    std::move(callback).Run();
  }
  pending_callbacks_.clear();
}

}  // namespace adblock
