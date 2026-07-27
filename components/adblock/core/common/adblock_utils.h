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
#include <vector>

#include "components/adblock/core/common/flatbuffer_data.h"
#include "url/gurl.h"

namespace adblock::utils {

struct DownloadStats {
  long success_count = 0;
  long error_count = 0;

  bool operator==(const DownloadStats& other) const {
    return success_count == other.success_count &&
           error_count == other.error_count;
  }
};

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input);

// Creates a FlatbufferData object that holds data from the ResourceBundle

std::unique_ptr<FlatbufferData> MakeFlatbufferDataFromResourceBundle(
    int resource_id);

bool RegexMatches(std::string_view pattern,
                  std::string_view input,
                  bool case_sensitive);

}  // namespace adblock::utils

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_UTILS_H_
