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

#include "components/adblock/core/features.h"

namespace adblock {

BASE_FEATURE(kAdblockPlusFeature,
             "AdblockPlus",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEyeoMlServiceFeature,
             "EyeoMlService",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace adblock
