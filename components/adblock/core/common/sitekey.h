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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_SITEKEY_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_SITEKEY_H_

#include "base/types/strong_alias.h"

namespace adblock {

using SiteKey = base::StrongAlias<class SiteKeyTag, std::string>;

}

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_SITEKEY_H_
