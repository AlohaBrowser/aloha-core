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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_INSTALLED_SUBSCRIPTION_IMPL_TEST_BASE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_INSTALLED_SUBSCRIPTION_IMPL_TEST_BASE_H_

#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplTestBase : public testing::Test {
 public:
  AdblockInstalledSubscriptionImplTestBase();
  ~AdblockInstalledSubscriptionImplTestBase() override;

  std::set<std::string_view> FilterSelectors(
      InstalledSubscription::ContentFiltersData selectors);

  scoped_refptr<InstalledSubscription> ConvertAndLoadRules(
      std::string rules,
      GURL url = GURL(),
      bool allow_privileged = false);

 public:
  scoped_refptr<FlatbufferConverter> converter_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_INSTALLED_SUBSCRIPTION_IMPL_TEST_BASE_H_
