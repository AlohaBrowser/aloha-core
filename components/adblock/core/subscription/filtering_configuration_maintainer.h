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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_H_

#include <memory>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_collection.h"

namespace adblock {

// Maintains a set of subscriptions needed to fulfil filtering requirements of a
// single FilteringConfiguration.
// Downloads and installs missing subscriptions, removes no-longer-needed
// subscriptions, periodically updates installed subscriptions.
class FilteringConfigurationMaintainer {
 public:
  virtual ~FilteringConfigurationMaintainer() = default;

  // Returns a SubscriptionCollection that implements the blocking logic
  // demanded by a FilteringConfiguration. This becomes part of a
  // SubscriptionService::Snapshot.
  virtual std::unique_ptr<SubscriptionCollection> GetSubscriptionCollection()
      const = 0;

  // Allows inspecting what Subscriptions are currently in use. This includes
  // ongoing downloads, preloaded subscriptions and installed subscriptions.
  virtual std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions()
      const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_FILTERING_CONFIGURATION_MAINTAINER_H_
