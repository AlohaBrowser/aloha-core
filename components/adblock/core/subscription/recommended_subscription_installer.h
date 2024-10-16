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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_H_

#include <vector>

#include "base/functional/callback_forward.h"
#include "url/gurl.h"

namespace adblock {

// Downloads, parses and installs recommended subscriptions based on
// geolocation.
class RecommendedSubscriptionInstaller {
 public:
  using RecommendedSubscriptionsParsedCallback =
      base::OnceCallback<void(const std::vector<GURL>&)>;

  virtual ~RecommendedSubscriptionInstaller() {}

  virtual void RunUpdateCheck() = 0;
  virtual void RemoveAutoInstalledSubscriptions() = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_H_
