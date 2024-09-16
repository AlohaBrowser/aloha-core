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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_H_

#include <string>

#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace adblock {

// Persistently stores metadata about Subscriptions.
// Metadata is data about Subscriptions that may not be encoded in the
// Subscriptions themselves, like number of errors encountered while
// downloading.
// Subscription metadata is used to control subscription update behavior and
// provide data for GET/HEAD query parameters.
class SubscriptionPersistentMetadata : public KeyedService {
 public:
  // Sets the expiration date to Now() + |expires_in|.
  // Expiration time can be:
  // - Parsed from the filter list (see Subscription::GetTimeUntilExpires())
  // - Set to 5 days by default, if expiration time is not specified in filter
  // list
  // - Set to 1 day by default for the special case of HEAD-only request.
  virtual void SetExpirationInterval(const GURL& subscription_url,
                                     base::TimeDelta expires_in) = 0;
  // Sets the last installation time to Now().
  virtual void SetLastInstallationTime(const GURL& subscription_url) = 0;
  // The version of a subscription can be:
  // - parsed from the filter list (see Subscription::GetCurrentVersion())
  // - for HEAD requests, created by parsing the received "Date" header.
  // - not set
  // It is common for custom subscriptions to not have a version available.
  virtual void SetVersion(const GURL& subscription_url,
                          std::string version) = 0;
  // Increments the total number of successful downloads.
  // Successful HEAD-only requests, which don't deliver an actual subscription,
  // still count towards this number.
  // Resets the download error count to 0, as it breaks the error streak.
  virtual void IncrementDownloadSuccessCount(const GURL& subscription_url) = 0;
  // Increments the number of consecutive download failures, used to determine
  // whether to fall back to an alternate download URL.
  // Incrementing the error count does *not* influence the success count.
  virtual void IncrementDownloadErrorCount(const GURL& subscription_url) = 0;
  // Returns whether the expiration time (see SetExpirationInterval()) is
  // earlier than Now().
  // A subscription for which SetExpirationInterval() was never called is
  // considered expired, as otherwise it would never be selected for updating.
  virtual bool IsExpired(const GURL& subscription_url) const = 0;
  // Returns time of the last installation set by SetLastInstallationTime().
  virtual base::Time GetLastInstallationTime(
      const GURL& subscription_url) const = 0;
  // Returns version set in SetVersion() or "0" when not set.
  // Subscriptions are allowed to not have a version defined.
  virtual std::string GetVersion(const GURL& subscription_url) const = 0;
  // Returns the number of successful downloads of this subscription in the
  // past.
  virtual int GetDownloadSuccessCount(const GURL& subscription_url) const = 0;
  // Returns number of consecutive download errors.
  virtual int GetDownloadErrorCount(const GURL& subscription_url) const = 0;
  // Mark the subscription as auto installed. Auto installed subscriptions have
  // a secondary expiration time, triggering the removal of the subscription if
  // it expires.
  virtual void SetAutoInstalledExpirationInterval(
      const GURL& subscription_url,
      base::TimeDelta expires_in) = 0;
  // Returns whether the subscription is auto installed.
  virtual bool IsAutoInstalled(const GURL& subscription_url) const = 0;
  // Returns whether the auto installed expiration time (see
  // SetAutoInstalledExpirationInterval()) is earlier than Now(). A subscription
  // for which SetAutoInstalledExpirationInterval() was never called is not
  // considered expired since that would trigger the removal of the subscription
  virtual bool IsAutoInstalledExpired(const GURL& subscription_url) const = 0;

  // Remove metadata associated with |subscription_url|.
  virtual void RemoveMetadata(const GURL& subscription_url) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_PERSISTENT_METADATA_H_
