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

#include "components/adblock/core/subscription/subscription_validator_impl.h"

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/sha2.h"

namespace adblock {
namespace {

std::string ComputeSubscriptionHash(const FlatbufferData& buffer) {
  return base::Base64Encode(crypto::SHA256Hash(buffer.span()));
}

// When the schema version used to create installed subscriptions is different
// from the schema version known by this browser, we should not attempt to read
// the flatbuffers - we would misinterpret their content.
// Clear the stored subscription signatures to indicate the files are invalid.
void ClearSignaturesIfSchemaVersionChanged(
    PrefService* pref_service,
    const std::string& current_schema_version) {
  if (pref_service->GetString(common::prefs::kLastUsedSchemaVersion) !=
      current_schema_version) {
    if (!pref_service->FindPreference(common::prefs::kSubscriptionSignatures)
             ->IsDefaultValue()) {
      LOG(INFO) << "[eyeo] Schema version has changed, invalidating stored "
                   "subscriptions.";
      pref_service->ClearPref(common::prefs::kSubscriptionSignatures);
    }
    pref_service->SetString(common::prefs::kLastUsedSchemaVersion,
                            current_schema_version);
  }
}

bool IsSignatureValidInternal(
    const base::Value::Dict& initial_subscription_signatures,
    const FlatbufferData& data,
    const base::FilePath& path) {
  const auto* expected_hash = initial_subscription_signatures.FindString(
      path.BaseName().AsUTF8Unsafe());
  if (!expected_hash) {
    DLOG(WARNING) << "[eyeo] " << path << " has no matching signature in prefs";
    return false;
  }
  if (*expected_hash != ComputeSubscriptionHash(data)) {
    DLOG(WARNING) << "[eyeo] " << path << " has invalid signature in prefs";
    return false;
  }
  return true;
}

void StoreTrustedSignatureInternal(
    scoped_refptr<base::TaskRunner> main_task_runner,
    base::OnceCallback<void(std::string signature, const base::FilePath& path)>
        signature_receiver,
    const FlatbufferData& data,
    const base::FilePath& path) {
  // Compute the hash on the current, background thread.
  const auto hash = ComputeSubscriptionHash(data);
  // Post the hash for storing into the main thread.
  main_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(signature_receiver), std::move(hash), path));
}

}  // namespace

SubscriptionValidatorImpl::SubscriptionValidatorImpl(
    PrefService* pref_service,
    const std::string& current_schema_version)
    : pref_service_(pref_service) {
  ClearSignaturesIfSchemaVersionChanged(pref_service_, current_schema_version);
}

SubscriptionValidatorImpl::~SubscriptionValidatorImpl() = default;

SubscriptionValidator::IsSignatureValidThreadSafeCallback
SubscriptionValidatorImpl::IsSignatureValid() const {
  return base::BindRepeating(
      &IsSignatureValidInternal,
      pref_service_->GetDict(common::prefs::kSubscriptionSignatures).Clone());
}

SubscriptionValidator::StoreTrustedSignatureThreadSafeCallback
SubscriptionValidatorImpl::StoreTrustedSignature() {
  return base::BindOnce(
      &StoreTrustedSignatureInternal,
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(
          &SubscriptionValidatorImpl::StoreTrustedSignatureOnMainThread,
          weak_ptr_factory_.GetWeakPtr()));
}

SubscriptionValidator::RemoveStoredSignatureThreadSafeCallback
SubscriptionValidatorImpl::RemoveStoredSignature() {
  return base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(
          &SubscriptionValidatorImpl::RemoveStoredSignatureInMainThread,
          weak_ptr_factory_.GetWeakPtr()));
}

void SubscriptionValidatorImpl::StoreTrustedSignatureOnMainThread(
    std::string signature,
    const base::FilePath& path) {
  ScopedDictPrefUpdate pref_update(pref_service_,
                                   common::prefs::kSubscriptionSignatures);
  const auto key = path.BaseName().AsUTF8Unsafe();
  pref_update->Set(key, base::Value(signature));
}

void SubscriptionValidatorImpl::RemoveStoredSignatureInMainThread(
    const base::FilePath& path) {
  ScopedDictPrefUpdate pref_update(pref_service_,
                                   common::prefs::kSubscriptionSignatures);
  const auto key = path.BaseName().AsUTF8Unsafe();
  pref_update->Remove(key);
}

}  // namespace adblock
