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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_IMPL_H_

#include <map>

#include "base/memory/raw_ptr.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/prefs/pref_service.h"

namespace adblock {

// Stores persistent subscription metadata in PrefService.
class SubscriptionPersistentMetadataImpl final
    : public SubscriptionPersistentMetadata {
 public:
  explicit SubscriptionPersistentMetadataImpl(PrefService* prefs);
  ~SubscriptionPersistentMetadataImpl() final;

  void SetExpirationInterval(const GURL& subscription_url,
                             base::TimeDelta expires_in) final;
  void SetLastInstallationTime(const GURL& subscription_url) final;
  void SetVersion(const GURL& subscription_url, std::string version) final;
  void IncrementDownloadSuccessCount(const GURL& subscription_url) final;
  void IncrementDownloadErrorCount(const GURL& subscription_url) final;

  bool IsExpired(const GURL& subscription_url) const final;
  base::Time GetLastInstallationTime(const GURL& subscription_url) const final;
  std::string GetVersion(const GURL& subscription_url) const final;
  int GetDownloadSuccessCount(const GURL& subscription_url) const final;
  int GetDownloadErrorCount(const GURL& subscription_url) const final;

  void SetAutoInstalledExpirationInterval(const GURL& subscription_url,
                                          base::TimeDelta expires_in) final;
  bool IsAutoInstalled(const GURL& subscription_url) const final;
  bool IsAutoInstalledExpired(const GURL& subscription_url) const final;

  void RemoveMetadata(const GURL& subscription_url) final;

 private:
  struct Metadata;
  using MetadataMap = std::map<GURL, Metadata>;
  void UpdatePrefs();
  void LoadFromPrefs();

  raw_ptr<PrefService> prefs_;
  MetadataMap metadata_map_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_IMPL_H_
