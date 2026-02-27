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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONTENT_USER_DATA_KEYS_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONTENT_USER_DATA_KEYS_H_

#include "base/component_export.h"

namespace adblock {

COMPONENT_EXPORT(ADBLOCK_CONTENT_KEYS)
extern const char kSubscriptionServiceUserDataKey[];

COMPONENT_EXPORT(ADBLOCK_CONTENT_KEYS)
extern const char kResourceClassificationRunnerUserDataKey[];

COMPONENT_EXPORT(ADBLOCK_CONTENT_KEYS)
extern const char kAdblockTelemetryServiceUserDataKey[];

COMPONENT_EXPORT(ADBLOCK_CONTENT_KEYS)
extern const char kSessionStatsServiceUserDataKey[];

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONTENT_USER_DATA_KEYS_H_
