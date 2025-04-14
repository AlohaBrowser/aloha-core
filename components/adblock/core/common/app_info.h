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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_APP_INFO_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_APP_INFO_H_

#include <string>

namespace adblock {

class AppInfo {
 public:
  static const AppInfo& Get();

  AppInfo(const AppInfo&) = delete;
  AppInfo& operator=(const AppInfo&) = delete;
  ~AppInfo() = default;

  std::string name;
  std::string client_os;
  std::string version;

 private:
  AppInfo();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_APP_INFO_H_
