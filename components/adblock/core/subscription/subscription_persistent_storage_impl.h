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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_IMPL_H_

#include <map>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/subscription_persistent_storage.h"
#include "components/adblock/core/subscription/subscription_validator.h"

namespace adblock {

class SubscriptionPersistentStorageImpl final
    : public SubscriptionPersistentStorage {
 public:
  SubscriptionPersistentStorageImpl(
      base::FilePath base_storage_dir,
      std::unique_ptr<SubscriptionValidator> validator,
      SubscriptionPersistentMetadata* persistent_metadata);
  ~SubscriptionPersistentStorageImpl() final;

  void LoadSubscriptions(LoadCallback on_loaded) final;
  void StoreSubscription(std::unique_ptr<FlatbufferData> raw_data,
                         StoreCallback on_finished) final;
  void RemoveSubscription(
      scoped_refptr<InstalledSubscription> subscription) final;

 private:
  using SubscriptionFileMapping =
      std::map<scoped_refptr<InstalledSubscription>, base::FilePath>;
  using LoadedBuffer =
      std::pair<std::unique_ptr<FlatbufferData>, base::FilePath>;
  static LoadedBuffer WriteSubscription(
      const base::FilePath& storage_dir,
      std::unique_ptr<FlatbufferData> raw_data,
      SubscriptionValidator::StoreTrustedSignatureThreadSafeCallback
          store_signature);
  static std::vector<LoadedBuffer> ReadSubscriptionsFromDirectory(
      const base::FilePath& storage_dir,
      SubscriptionValidator::IsSignatureValidThreadSafeCallback
          is_signature_valid);
  void LoadComplete(LoadCallback on_initialized,
                    std::vector<LoadedBuffer> loaded_buffers);
  void SubscriptionStored(StoreCallback on_finished,
                          LoadedBuffer storage_result);

  SEQUENCE_CHECKER(sequence_checker_);
  base::FilePath base_storage_dir_;
  std::unique_ptr<SubscriptionValidator> validator_;
  raw_ptr<SubscriptionPersistentMetadata> persistent_metadata_;
  // Maps Subscriptions to files that they access.
  SubscriptionFileMapping backing_file_mapping_;
  base::WeakPtrFactory<SubscriptionPersistentStorageImpl> weak_ptr_factory{
      this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_STORAGE_IMPL_H_
