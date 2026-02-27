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

#include "components/adblock/content/browser/eyeo_page_info.h"

namespace adblock {

PAGE_USER_DATA_KEY_IMPL(EyeoPageInfo);

EyeoPageInfo::EyeoPageInfo(content::Page& page)
    : content::PageUserData<EyeoPageInfo>(page) {}

EyeoPageInfo::~EyeoPageInfo() = default;

bool EyeoPageInfo::SetMatchedPageView(PageViewStats::Metric metric) {
  auto result = page_views_.insert(metric);
  return result.second;
}

bool EyeoPageInfo::HasMatchedPageView(PageViewStats::Metric metric) {
  return page_views_.count(metric);
}

}  // namespace adblock
