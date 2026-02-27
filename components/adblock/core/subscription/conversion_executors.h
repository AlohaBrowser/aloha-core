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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_CONVERSION_EXECUTORS_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_CONVERSION_EXECUTORS_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "components/adblock/core/subscription/installed_subscription.h"

#include "url/gurl.h"

namespace adblock {

class ConversionExecutors {
 public:
  // Synchronous
  virtual scoped_refptr<InstalledSubscription> ConvertCustomFilters(
      const std::vector<std::string>& filters) const = 0;
  // Asynchronous
  virtual void ConvertFilterListFile(
      const GURL& subscription_url,
      const base::FilePath& path,
      base::OnceCallback<void(ConversionResult)> result_callback) const = 0;

  virtual ~ConversionExecutors() = default;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_CONVERSION_EXECUTORS_H_
