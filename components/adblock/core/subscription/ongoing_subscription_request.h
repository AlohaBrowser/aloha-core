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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_ONGOING_SUBSCRIPTION_REQUEST_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_ONGOING_SUBSCRIPTION_REQUEST_H_

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace adblock {

// State machine of a download request of a single subscription (GET or HEAD).
// It implements observing network state and retries.
class OngoingSubscriptionRequest {
 public:
  using ResponseCallback = base::RepeatingCallback<void(
      const GURL& subscription_url,
      base::FilePath downloaded_file,
      scoped_refptr<net::HttpResponseHeaders> headers)>;
  enum class Method { GET, HEAD };

  virtual ~OngoingSubscriptionRequest() = default;

  virtual void Start(GURL url,
                     Method method,
                     ResponseCallback response_callback) = 0;
  virtual void Retry() = 0;
  virtual void Redirect(GURL redirect_url) = 0;

  virtual size_t GetNumberOfRedirects() const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_ONGOING_SUBSCRIPTION_REQUEST_H_
