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

#include "components/adblock/core/net/test/mock_adblock_request_throttle.h"

namespace adblock {

MockAdblockRequestThrottle::MockAdblockRequestThrottle() = default;

MockAdblockRequestThrottle::~MockAdblockRequestThrottle() = default;

void MockAdblockRequestThrottle::RunWhenRequestsAllowed(
    base::OnceClosure callback) {
  if (!requests_allowed_) {
    pending_callbacks_.push_back(std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void MockAdblockRequestThrottle::AllowRequestsAfter(base::TimeDelta delay) {}

void MockAdblockRequestThrottle::OverrideDelayImmediatelyForTesting() {
  requests_allowed_ = true;
  for (auto& callback : pending_callbacks_) {
    std::move(callback).Run();
  }
  pending_callbacks_.clear();
}

}  // namespace adblock
