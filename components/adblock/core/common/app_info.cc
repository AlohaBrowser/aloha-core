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

#include "components/adblock/core/common/app_info.h"

#include "base/strings/string_util.h"
#include "components/version_info/version_info.h"

namespace adblock {

// static
const AppInfo& AppInfo::Get() {
  static AppInfo instance;
  return instance;
}

AppInfo::AppInfo() {
#if defined(EYEO_APPLICATION_NAME)
  name = EYEO_APPLICATION_NAME;
#else
  name = version_info::GetProductName();
#endif
#if defined(EYEO_APPLICATION_VERSION)
  version = EYEO_APPLICATION_VERSION;
#else
  version = version_info::GetVersionNumber();
#endif
  base::ReplaceChars(version_info::GetOSType(), base::kWhitespaceASCII, "",
                     &client_os);
}

}  // namespace adblock
