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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONSTANTS_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONSTANTS_H_

#include <string_view>

#include "base/auto_reset.h"
#include "url/gurl.h"

namespace adblock {

namespace flat {
enum AbpResource : int8_t;
}

extern const char kSiteKeyHeaderKey[];
extern const char kAllowlistEverythingFilter[];
extern const char kAdblockFilteringConfigurationName[];

const std::string& CurrentSchemaVersion();
const GURL& TestPagesSubscriptionUrl();
const GURL& CustomFiltersUrl();
std::string_view RewriteUrl(flat::AbpResource type);

bool IsEyeoFilteringDisabledByDefault();
bool IsAcceptableAdsDisabledByDefault();

// Override result of IsEyeoFilteringDisabledByDefault() for the
// duration of the returned AutoReset's lifetime. Used for testing.
base::AutoReset<bool> OverrideEyeoFilteringDisabledByDefault(bool val);

// Override result of IsAcceptableAdsDisabledByDefault() for the
// duration of the returned AutoReset's lifetime. Used for testing.
base::AutoReset<bool> OverrideAcceptableAdsDisabledByDefault(bool val);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_CONSTANTS_H_
