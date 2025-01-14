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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_

#include <string_view>
#include <vector>

class PrefRegistrySimple;

namespace adblock::common::prefs {

extern const char kEnableAdblockLegacy[];
extern const char kEnableAcceptableAdsLegacy[];
extern const char kAdblockAllowedDomainsLegacy[];
extern const char kAdblockCustomFiltersLegacy[];
extern const char kAdblockSubscriptionsLegacy[];
extern const char kAdblockCustomSubscriptionsLegacy[];
extern const char kAdblockMoreOptionsEnabled[];
extern const char kInstallFirstStartSubscriptions[];
extern const char kSubscriptionSignatures[];
extern const char kLastUsedSchemaVersion[];
extern const char kSubscriptionMetadata[];
extern const char kTelemetryLastPingTag[];
extern const char kTelemetryLastPingTime[];
extern const char kTelemetryPreviousLastPingTime[];
extern const char kTelemetryFirstPingTime[];
extern const char kTelemetryNextPingTime[];
extern const char kTelemetryPageViewStats[];
extern const char kConfigurationsPrefsPath[];
extern const char kEnableAutoInstalledSubscriptions[];
extern const char kAutoInstalledSubscriptionsNextUpdateTime[];

void RegisterProfilePrefs(PrefRegistrySimple* registry);

std::vector<std::string_view> GetPrefs();

}  // namespace adblock::common::prefs

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_
