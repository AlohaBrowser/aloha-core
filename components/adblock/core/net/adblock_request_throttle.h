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

#ifndef COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_H_
#define COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_H_

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"

namespace adblock {

// Centralized throttle to prohibit network requests from executing too early
// after browser startup or in other situations that might require a delay.
class AdblockRequestThrottle : public KeyedService {
 public:
  // Runs |callback| when requests become allowed, or immediately if they're
  // already allowed.
  virtual void RunWhenRequestsAllowed(base::OnceClosure callback) = 0;

  // Starts a timer that will allow requests to be made after |delay|.
  // Typically called once, shortly after browser startup, but can be called
  // multiple times to extend or shorten the delay - this would mostly be used
  // for browser tests.
  // |delay| of zero means requests are allowed immediately.
  virtual void AllowRequestsAfter(base::TimeDelta delay) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_NET_ADBLOCK_REQUEST_THROTTLE_H_
