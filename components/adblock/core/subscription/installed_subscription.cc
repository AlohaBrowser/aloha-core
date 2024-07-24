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

#include "components/adblock/core/subscription/installed_subscription.h"

namespace adblock {

InstalledSubscription::ContentFiltersData::ContentFiltersData() = default;
InstalledSubscription::ContentFiltersData::~ContentFiltersData() = default;
InstalledSubscription::ContentFiltersData::ContentFiltersData(
    const ContentFiltersData&) = default;
InstalledSubscription::ContentFiltersData::ContentFiltersData(
    ContentFiltersData&&) = default;
InstalledSubscription::ContentFiltersData&
InstalledSubscription::ContentFiltersData::operator=(
    const ContentFiltersData&) = default;
InstalledSubscription::ContentFiltersData&
InstalledSubscription::ContentFiltersData::operator=(ContentFiltersData&&) =
    default;

InstalledSubscription::Snippet::Snippet() = default;
InstalledSubscription::Snippet::Snippet(const Snippet&) = default;
InstalledSubscription::Snippet::Snippet(Snippet&&) = default;
InstalledSubscription::Snippet::~Snippet() = default;
InstalledSubscription::Snippet& InstalledSubscription::Snippet::operator=(
    const Snippet&) = default;
InstalledSubscription::Snippet& InstalledSubscription::Snippet::operator=(
    Snippet&&) = default;

InstalledSubscription::~InstalledSubscription() = default;

}  // namespace adblock
