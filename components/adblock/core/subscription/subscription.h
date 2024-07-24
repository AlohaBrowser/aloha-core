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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace adblock {

// Represents a single filter list, ex. Easylist French or Acceptable Ads.
// Read-only and thread-safe.
class Subscription : public base::RefCountedThreadSafe<Subscription> {
 public:
  enum class InstallationState {
    // Subscription is installed and in use.
    Installed,
    // A preloaded version of this subscription is in use, a full version is
    // likely being downloaded from the Internet.
    Preloaded,
    // Subscription is being downloaded and not yet in use. No preloaded
    // substitute is available.
    Installing,
    // State is unknown when FilteringConfiguration is disabled.
    Unknown,
  };
  // Returns the URL of the text version of the subscription, ex.
  // https://easylist-downloads.adblockplus.org/easylist.txt.
  // Note that this may be different than the URL from which the subscription
  // was downloaded.
  virtual GURL GetSourceUrl() const = 0;

  // Returns the value of the `! Title:` field of the filter list, ex. "EasyList
  // Germany+EasyList". This is an optional field and may be empty.
  virtual std::string GetTitle() const = 0;

  // Returns the value of the `! Version:` field of the filter list, ex.
  // "202108191121". This is an optional field and may be empty.
  virtual std::string GetCurrentVersion() const = 0;

  // Returns whether this subscription is installed and in use, or whether it's
  // still being downloaded.
  virtual InstallationState GetInstallationState() const = 0;

  // Returns the time the subscription was installed or last updated.
  // Only valid when GetInstallationState() returns Installed, otherwise zero.
  virtual base::Time GetInstallationTime() const = 0;

  // Returns amount of time until subscription expires.
  // Typically, update checks are performed once per expiration interval.
  virtual base::TimeDelta GetExpirationInterval() const = 0;

 protected:
  friend class base::RefCountedThreadSafe<Subscription>;
  virtual ~Subscription();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_H_
