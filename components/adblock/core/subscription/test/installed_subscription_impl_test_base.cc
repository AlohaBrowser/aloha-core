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

#include "components/adblock/core/subscription/test/installed_subscription_impl_test_base.h"

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

AdblockInstalledSubscriptionImplTestBase::
    AdblockInstalledSubscriptionImplTestBase()
    : converter_(base::MakeRefCounted<FlatbufferConverter>()) {}

AdblockInstalledSubscriptionImplTestBase::
    ~AdblockInstalledSubscriptionImplTestBase() = default;

std::set<std::string_view>
AdblockInstalledSubscriptionImplTestBase::FilterSelectors(
    InstalledSubscription::ContentFiltersData selectors) {
  // Remove exceptions.
  selectors.elemhide_selectors.erase(
      std::remove_if(selectors.elemhide_selectors.begin(),
                     selectors.elemhide_selectors.end(),
                     [&](const auto& selector) {
                       return std::find(selectors.elemhide_exceptions.begin(),
                                        selectors.elemhide_exceptions.end(),
                                        selector) !=
                              selectors.elemhide_exceptions.end();
                     }),
      selectors.elemhide_selectors.end());
  std::set<std::string_view> filtered_selectors(
      selectors.elemhide_selectors.begin(), selectors.elemhide_selectors.end());
  return filtered_selectors;
}

scoped_refptr<InstalledSubscription>
AdblockInstalledSubscriptionImplTestBase::ConvertAndLoadRules(
    std::string rules,
    GURL url,
    bool allow_privileged) {
  // Without [Adblock Plus 2.0], the file is not a valid filter list.
  rules = "[Adblock Plus 2.0]\n! Title: TestingList\n" + rules;
  std::istringstream input(std::move(rules));
  auto converter_result = converter_->Convert(input, url, allow_privileged);
  if (!absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
          converter_result)) {
    return {};
  }
  return base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(absl::get<std::unique_ptr<FlatbufferData>>(converter_result)),
      Subscription::InstallationState::Installed, base::Time());
}

}  // namespace adblock
