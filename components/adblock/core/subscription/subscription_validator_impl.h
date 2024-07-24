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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/adblock/core/subscription/subscription_validator.h"
#include "components/prefs/pref_service.h"

class PrefService;

namespace adblock {

// Stores the hash of FlatbufferData in a Tracked Pref.
class SubscriptionValidatorImpl final : public SubscriptionValidator {
 public:
  SubscriptionValidatorImpl(PrefService* pref_service,
                            const std::string& current_schema_version);
  ~SubscriptionValidatorImpl() final;

  IsSignatureValidThreadSafeCallback IsSignatureValid() const final;
  StoreTrustedSignatureThreadSafeCallback StoreTrustedSignature() final;
  RemoveStoredSignatureThreadSafeCallback RemoveStoredSignature() final;

 private:
  void StoreTrustedSignatureOnMainThread(std::string signature,
                                         const base::FilePath& path);
  void RemoveStoredSignatureInMainThread(const base::FilePath& path);

  raw_ptr<PrefService> pref_service_;
  base::WeakPtrFactory<SubscriptionValidatorImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_
