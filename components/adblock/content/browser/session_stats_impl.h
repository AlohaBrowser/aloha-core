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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_SESSION_STATS_IMPL_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_SESSION_STATS_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/session_stats.h"

namespace adblock {

class SessionStatsImpl final : public SessionStats,
                               public ResourceClassificationRunner::Observer {
 public:
  explicit SessionStatsImpl(
      ResourceClassificationRunner* classification_runner);

  ~SessionStatsImpl() final;

  std::map<GURL, long> GetSessionAllowedResourcesCount() const final;

  std::map<GURL, long> GetSessionBlockedResourcesCount() const final;

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) final;
  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) final;
  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) final;

 private:
  void OnMatchedInternal(FilterMatchResult match_result,
                         const GURL& subscription);

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<ResourceClassificationRunner> classification_runner_;
  std::map<GURL, long> allowed_map_;
  std::map<GURL, long> blocked_map_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_SESSION_STATS_IMPL_H_
