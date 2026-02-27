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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_PAGE_INFO_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_PAGE_INFO_H_

#include <set>

#include "components/adblock/content/browser/page_view_stats.h"
#include "content/public/browser/page.h"
#include "content/public/browser/page_user_data.h"
#include "url/gurl.h"

namespace adblock {

class EyeoPageInfo final : public content::PageUserData<EyeoPageInfo> {
 public:
  ~EyeoPageInfo() final;

  // Marks that document has matched specified page view metric.
  // |key| is a base::PersistentHash() of a metric name.
  // Returns true if |key| was not yet stored for this document.
  bool SetMatchedPageView(PageViewStats::Metric metric);

  // Checks whether |key| is already stored.
  bool HasMatchedPageView(PageViewStats::Metric metric);

 private:
  explicit EyeoPageInfo(content::Page& page);

  std::set<PageViewStats::Metric> page_views_ = {};

  friend PageUserData;
  PAGE_USER_DATA_KEY_DECL();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_EYEO_PAGE_INFO_H_
