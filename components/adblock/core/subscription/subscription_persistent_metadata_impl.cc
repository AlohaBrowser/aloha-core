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

#include "components/adblock/core/subscription/subscription_persistent_metadata_impl.h"

#include <string_view>

#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace adblock {
namespace {
constexpr std::string_view kExpirationTimeKey = "expiration_time";
constexpr std::string_view kLastInstallationTimeKey = "last_installation_time";
constexpr std::string_view kVersionKey = "version";
constexpr std::string_view kDownloadCountKey = "download_count";
constexpr std::string_view kErrorCountKey = "error_count";
}  // namespace

struct SubscriptionPersistentMetadataImpl::Metadata {
  base::Time expiration_time;
  base::Time last_installation_time;
  std::string version{"0"};
  int download_count{0};
  int error_count{0};
};

SubscriptionPersistentMetadataImpl::SubscriptionPersistentMetadataImpl(
    PrefService* prefs)
    : prefs_(prefs) {
  LoadFromPrefs();
}

SubscriptionPersistentMetadataImpl::~SubscriptionPersistentMetadataImpl() =
    default;

void SubscriptionPersistentMetadataImpl::SetExpirationInterval(
    const GURL& subscription_url,
    base::TimeDelta expires_in) {
  const auto now = base::Time::Now();
  metadata_map_[subscription_url].expiration_time = now + expires_in;
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::SetLastInstallationTime(
    const GURL& subscription_url) {
  metadata_map_[subscription_url].last_installation_time = base::Time::Now();
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::SetVersion(
    const GURL& subscription_url,
    std::string version) {
  metadata_map_[subscription_url].version = std::move(version);
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::IncrementDownloadSuccessCount(
    const GURL& subscription_url) {
  metadata_map_[subscription_url].download_count++;
  metadata_map_[subscription_url].error_count = 0;
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::IncrementDownloadErrorCount(
    const GURL& subscription_url) {
  metadata_map_[subscription_url].error_count++;
  UpdatePrefs();
}

bool SubscriptionPersistentMetadataImpl::IsExpired(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end()) {
    return true;
  }
  return it->second.expiration_time <= base::Time::Now();
}

base::Time SubscriptionPersistentMetadataImpl::GetLastInstallationTime(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end()) {
    return base::Time();
  }
  return it->second.last_installation_time;
}

std::string SubscriptionPersistentMetadataImpl::GetVersion(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end()) {
    return "0";
  }
  return it->second.version;
}

int SubscriptionPersistentMetadataImpl::GetDownloadSuccessCount(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end()) {
    return 0;
  }
  return it->second.download_count;
}

int SubscriptionPersistentMetadataImpl::GetDownloadErrorCount(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end()) {
    return 0;
  }
  return it->second.error_count;
}

void SubscriptionPersistentMetadataImpl::RemoveMetadata(
    const GURL& subscription_url) {
  metadata_map_.erase(subscription_url);
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::UpdatePrefs() {
  base::Value::Dict dict;
  for (const auto& pair : metadata_map_) {
    base::Value::Dict subscription;
    subscription.Set(kExpirationTimeKey,
                     TimeToValue(pair.second.expiration_time));
    subscription.Set(kLastInstallationTimeKey,
                     TimeToValue(pair.second.last_installation_time));
    subscription.Set(kVersionKey, pair.second.version);
    subscription.Set(kDownloadCountKey, pair.second.download_count);
    subscription.Set(kErrorCountKey, pair.second.error_count);
    dict.Set(pair.first.spec(), std::move(subscription));
  }
  prefs_->SetDict(common::prefs::kSubscriptionMetadata, std::move(dict));
}

void SubscriptionPersistentMetadataImpl::LoadFromPrefs() {
  const base::Value& dict =
      prefs_->GetValue(common::prefs::kSubscriptionMetadata);
  DCHECK(dict.is_dict());
  for (const auto dict_item : dict.GetDict()) {
    Metadata subscription;
    DCHECK(dict_item.second.is_dict());
    const auto* d = dict_item.second.GetIfDict();
    subscription.expiration_time =
        ValueToTime(d->Find(kExpirationTimeKey)).value_or(base::Time());
    subscription.last_installation_time =
        ValueToTime(d->Find(kLastInstallationTimeKey)).value_or(base::Time());
    const auto* version = d->FindString(kVersionKey);
    if (version) {
      subscription.version = *version;
    }
    subscription.error_count = d->FindInt(kErrorCountKey).value_or(0);
    subscription.download_count = d->FindInt(kDownloadCountKey).value_or(0);
    metadata_map_.emplace(dict_item.first, std::move(subscription));
  }
}

}  // namespace adblock
