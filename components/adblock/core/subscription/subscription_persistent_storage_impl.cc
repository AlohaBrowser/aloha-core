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

#include "components/adblock/core/subscription/subscription_persistent_storage_impl.h"

#include <algorithm>
#include <iterator>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/common/trace_event_common.h"
#include "base/trace_event/trace_event.h"
#include "base/unguessable_token.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_validator.h"

namespace adblock {
namespace {

GURL UrlFromFlatbufferData(const FlatbufferData& flatbuffer) {
  DCHECK(flat::GetSubscription(flatbuffer.data())->metadata());
  DCHECK(flat::GetSubscription(flatbuffer.data())->metadata()->url());
  return GURL(
      flat::GetSubscription(flatbuffer.data())->metadata()->url()->str());
}

}  // namespace

SubscriptionPersistentStorageImpl::SubscriptionPersistentStorageImpl(
    base::FilePath base_storage_dir,
    std::unique_ptr<SubscriptionValidator> validator,
    SubscriptionPersistentMetadata* persistent_metadata)
    : base_storage_dir_(std::move(base_storage_dir)),
      validator_(std::move(validator)),
      persistent_metadata_(persistent_metadata) {}

SubscriptionPersistentStorageImpl::~SubscriptionPersistentStorageImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
SubscriptionPersistentStorageImpl::LoadedBuffer
SubscriptionPersistentStorageImpl::WriteSubscription(
    const base::FilePath& storage_dir,
    std::unique_ptr<FlatbufferData> raw_data,
    SubscriptionValidator::StoreTrustedSignatureThreadSafeCallback
        store_signature) {
  // To avoid conflict between existing subscription files, generate a new
  // unique path.
  base::FilePath subscription_path = storage_dir.AppendASCII(
      base::UnguessableToken::Create().ToString() + ".fb");
  // UnguessableToken is a 128-bit, cryptographically-safe random number,
  // conflicts are less likely than disk failure. The DCHECK is to express
  // intent.
  DCHECK(!base::PathExists(subscription_path));
  if (base::WriteFile(subscription_path,
                      reinterpret_cast<const char*>(raw_data->data()),
                      raw_data->size()) == -1) {
    // Disk write failed.
    return std::make_pair(nullptr, base::FilePath{});
  }
  auto buffer = std::make_unique<MemoryMappedFlatbufferData>(subscription_path);
  if (!buffer->data()) {
    // Creating the memory-mapped region failed.
    // TODO(DPD-1278) revert to in-memory buffer?
    return std::make_pair(nullptr, base::FilePath{});
  }
  std::move(store_signature).Run(*buffer, subscription_path);
  return std::make_pair(std::move(buffer), std::move(subscription_path));
}

// static
std::vector<SubscriptionPersistentStorageImpl::LoadedBuffer>
SubscriptionPersistentStorageImpl::ReadSubscriptionsFromDirectory(
    const base::FilePath& storage_dir,
    SubscriptionValidator::IsSignatureValidThreadSafeCallback
        is_signature_valid) {
  DLOG(INFO) << "[eyeo] Reading subscriptions from directory";
  TRACE_EVENT0("eyeo", "ReadSubscriptionsFromDirectory");
  // Does nothing if directory already exists:
  base::CreateDirectory(storage_dir);

  std::vector<SubscriptionPersistentStorageImpl::LoadedBuffer> result;
  base::FileEnumerator enumerator(storage_dir, false /* recursive */,
                                  base::FileEnumerator::FILES);
  // Iterate through |storage_dir| and try to load all files within.
  for (base::FilePath flatbuffer_path = enumerator.Next();
       !flatbuffer_path.empty(); flatbuffer_path = enumerator.Next()) {
    std::string contents;

    TRACE_EVENT_BEGIN1("eyeo", "ReadFileToString", "path",
                       flatbuffer_path.AsUTF8Unsafe());
    if (!base::ReadFileToString(flatbuffer_path, &contents)) {
      // File could not be read.
      base::DeleteFile(flatbuffer_path);
      continue;
    }
    TRACE_EVENT_END1("eyeo", "ReadFileToString", "path",
                     flatbuffer_path.AsUTF8Unsafe());
    TRACE_EVENT_BEGIN0("eyeo", "VerifySubscriptionBuffer");
    if (!is_signature_valid.Run(InMemoryFlatbufferData(std::move(contents)),
                                flatbuffer_path)) {
      // This is not a valid subscription file, remove it.
      base::DeleteFile(flatbuffer_path);
      continue;
    }
    TRACE_EVENT_END0("eyeo", "VerifySubscriptionBuffer");
    auto buffer = std::make_unique<MemoryMappedFlatbufferData>(flatbuffer_path);
    if (!buffer->data()) {
      // Could not create mapped memory region to file content.
      // TODO(mpawlowski) revert to in-memory buffer?
      continue;
    }
    result.emplace_back(std::move(buffer), std::move(flatbuffer_path));
  }
  DLOG(INFO) << "[eyeo] Finished reading and validating subscriptions. Loaded "
             << result.size() << " subscriptions.";
  return result;
}

void SubscriptionPersistentStorageImpl::LoadSubscriptions(
    LoadCallback on_loaded) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_ASYNC_BEGIN0(
      "eyeo", "SubscriptionPersistentStorageImpl::LoadSubscription",
      TRACE_ID_LOCAL(this));
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadSubscriptionsFromDirectory, base_storage_dir_,
                     validator_->IsSignatureValid()),
      base::BindOnce(&SubscriptionPersistentStorageImpl::LoadComplete,
                     weak_ptr_factory.GetWeakPtr(), std::move(on_loaded)));
}

void SubscriptionPersistentStorageImpl::StoreSubscription(
    std::unique_ptr<FlatbufferData> raw_data,
    StoreCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&WriteSubscription, base_storage_dir_, std::move(raw_data),
                     validator_->StoreTrustedSignature()),
      base::BindOnce(&SubscriptionPersistentStorageImpl::SubscriptionStored,
                     weak_ptr_factory.GetWeakPtr(), std::move(on_finished)));
}

void SubscriptionPersistentStorageImpl::RemoveSubscription(
    scoped_refptr<InstalledSubscription> subscription) {
  auto it = backing_file_mapping_.find(subscription);
  DCHECK(it != backing_file_mapping_.end())
      << "Attempted to remove subscription not governed by this "
         "SubscriptionPersistentStorageImpl";
  validator_->RemoveStoredSignature().Run(it->second);
  backing_file_mapping_.erase(it);
  subscription->MarkForPermanentRemoval();
}

void SubscriptionPersistentStorageImpl::LoadComplete(
    LoadCallback on_loaded,
    std::vector<LoadedBuffer> loaded_buffers) {
  std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions;
  for (LoadedBuffer& loaded_buffer : loaded_buffers) {
    const auto url = UrlFromFlatbufferData(*loaded_buffer.first);
    const auto last_installation_time =
        persistent_metadata_->GetLastInstallationTime(url);
    auto installed_subscription =
        base::MakeRefCounted<InstalledSubscriptionImpl>(
            std::move(loaded_buffer.first),
            Subscription::InstallationState::Installed, last_installation_time);
    backing_file_mapping_[installed_subscription] =
        std::move(loaded_buffer.second);
    loaded_subscriptions.push_back(installed_subscription);
  }
  TRACE_EVENT_ASYNC_END0("eyeo",
                         "SubscriptionPersistentStorageImpl::LoadSubscription",
                         TRACE_ID_LOCAL(this));
  std::move(on_loaded).Run(std::move(loaded_subscriptions));
}

void SubscriptionPersistentStorageImpl::SubscriptionStored(
    StoreCallback on_finished,
    LoadedBuffer storage_result) {
  if (!storage_result.first) {
    // There was an error storing the subscription.
    std::move(on_finished).Run(nullptr);
    return;
  }

  const auto url = UrlFromFlatbufferData(*storage_result.first);
  const auto last_installation_time = base::Time::Now();
  auto installed_subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(storage_result.first),
      Subscription::InstallationState::Installed, last_installation_time);
  persistent_metadata_->IncrementDownloadSuccessCount(url);
  persistent_metadata_->SetLastInstallationTime(url);
  persistent_metadata_->SetExpirationInterval(
      url, installed_subscription->GetExpirationInterval());
  const auto parsed_version = installed_subscription->GetCurrentVersion();
  if (!parsed_version.empty()) {
    persistent_metadata_->SetVersion(url, parsed_version);
  }
  backing_file_mapping_[installed_subscription] =
      std::move(storage_result.second);
  std::move(on_finished).Run(installed_subscription);
}

}  // namespace adblock
