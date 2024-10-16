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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_H_

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "components/adblock/core/common/flatbuffer_data.h"

namespace adblock {

// Validates potentially untrusted Subscriptions read from disk.
// Is thread-safe, returned callbacks can be called from a background thread.
class SubscriptionValidator {
 public:
  virtual ~SubscriptionValidator() = default;
  // Verifies if |data| has a signature that matches a previously stored
  // signature for |path| and whether the schema version is supported. To avoid
  // race conditions, only the state current for the time of retrieving the
  // callback is considered, subsequent calls to |StoreTrustedSignature| will
  // not affect the results. You need to recreate the callback to read new
  // state.
  using IsSignatureValidThreadSafeCallback =
      base::RepeatingCallback<bool(const FlatbufferData& data,
                                   const base::FilePath& path)>;
  virtual IsSignatureValidThreadSafeCallback IsSignatureValid() const = 0;

  // Asynchronously persistently store the signature of |data| associated with
  // |path|.
  using StoreTrustedSignatureThreadSafeCallback =
      base::OnceCallback<void(const FlatbufferData& data,
                              const base::FilePath& path)>;
  virtual StoreTrustedSignatureThreadSafeCallback StoreTrustedSignature() = 0;

  // Asynchronously removes the signature of file |path| from persistent
  // storage.
  using RemoveStoredSignatureThreadSafeCallback =
      base::OnceCallback<void(const base::FilePath& path)>;
  virtual RemoveStoredSignatureThreadSafeCallback RemoveStoredSignature() = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_H_
