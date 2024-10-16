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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/installed_subscription.h"

namespace adblock {

// Provides a persistent, disk-based storage for installed subscription files.
class SubscriptionPersistentStorage {
 public:
  virtual ~SubscriptionPersistentStorage() = default;
  using LoadCallback = base::OnceCallback<void(
      std::vector<scoped_refptr<InstalledSubscription>>)>;
  // Loads subscriptions from a directory on disk and returns them via
  // |on_loaded|.
  virtual void LoadSubscriptions(LoadCallback on_loaded) = 0;

  using StoreCallback =
      base::OnceCallback<void(scoped_refptr<InstalledSubscription>)>;
  // Stores |raw_data| to disk and returns a Subscription created from
  // flatbuffer parsed from |raw_data|.
  // |on_finished| gets called after the store to disk and parsing has finished,
  // nullptr argument signifies there was an error.
  // |raw_data| is assumed to be valid against the current flatbuffer schema, it
  // is not validated internally for performance reasons. Validate flatbuffers
  // downloaded from the Internet externally.
  virtual void StoreSubscription(std::unique_ptr<FlatbufferData> raw_data,
                                 StoreCallback on_finished) = 0;

  // Removes |subscription|'s file from disk.
  virtual void RemoveSubscription(
      scoped_refptr<InstalledSubscription> subscription) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_H_
