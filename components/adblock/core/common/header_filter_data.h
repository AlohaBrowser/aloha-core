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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_HEADER_FILTER_DATA_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_HEADER_FILTER_DATA_H_

#include <string_view>

#include "url/gurl.h"

namespace adblock {

struct HeaderFilterData {
  std::string_view header_filter;
  GURL subscription_url;
  // required by std::set
  bool operator<(const HeaderFilterData& other) const {
    return (header_filter < other.header_filter);
  }
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_HEADER_FILTER_DATA_H_
