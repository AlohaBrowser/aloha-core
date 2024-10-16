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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_PRELOADED_SUBSCRIPTION_PROVIDER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_PRELOADED_SUBSCRIPTION_PROVIDER_H_

#include <vector>

#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace adblock {

// Provides temporary preloaded subscriptions when needed.
// Preloaded subscriptions are filter lists bundled with the browser. They can
// be used to provide some level of ad-filtering while waiting for the download
// of up-to-date filter lists from the Internet.
class PreloadedSubscriptionProvider {
 public:
  virtual ~PreloadedSubscriptionProvider() = default;

  // The collection of preloaded subscriptions returned by
  // |GetCurrentPreloadedSubscriptions()| is built by comparing the list of
  // installed (ie. available) subscriptions with the list of pending (ie.
  // desired) subscriptions.
  virtual void UpdateSubscriptions(std::vector<GURL> installed_subscriptions,
                                   std::vector<GURL> pending_subscriptions) = 0;

  // Returns preloaded subscriptions that were deemed necessary, based on the
  // difference between pending and installed subscriptions, to provide relevant
  // temporary ad-filtering. This may include easylist.txt and
  // exceptionrules.txt. The subscriptions are kept in memory and released when
  // no longer needed.
  virtual std::vector<scoped_refptr<InstalledSubscription>>
  GetCurrentPreloadedSubscriptions() const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_PRELOADED_SUBSCRIPTION_PROVIDER_H_
