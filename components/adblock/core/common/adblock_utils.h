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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_UTILS_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_UTILS_H_

#include <string>
#include <string_view>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/common/sitekey.h"

class GURL;

namespace net {
class HttpResponseHeaders;
}

namespace adblock {

class Subscription;

namespace utils {

struct AppInfo {
  AppInfo();
  ~AppInfo();
  AppInfo(const AppInfo&);
  std::string name;
  std::string version;
  std::string client_os;
};

std::string CreateDomainAllowlistingFilter(const std::string& domain);

SiteKey GetSitekeyHeader(
    const scoped_refptr<net::HttpResponseHeaders>& headers);

AppInfo GetAppInfo();

std::string SerializeLanguages(const std::vector<std::string> languages);

std::vector<std::string> DeserializeLanguages(const std::string languages);

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input);

// converts |date| into abp version format ex: 202107210821
// in UTC format as necessary for server

std::string ConvertBaseTimeToABPFilterVersionFormat(const base::Time& date);

// Creates a FlatbufferData object that holds data from the ResourceBundle

std::unique_ptr<FlatbufferData> MakeFlatbufferDataFromResourceBundle(
    int resource_id);

bool RegexMatches(std::string_view pattern,
                  std::string_view input,
                  bool case_sensitive);

}  // namespace utils
}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_UTILS_H_
