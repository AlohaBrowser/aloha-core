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

#include "components/adblock/content/browser/session_stats_impl.h"

#include "components/adblock/core/common/adblock_constants.h"
#include "content/public/browser/browser_thread.h"

namespace adblock {

SessionStatsImpl::SessionStatsImpl(
    ResourceClassificationRunner* classification_runner)
    : classification_runner_(classification_runner) {
  DCHECK(classification_runner_);
  classification_runner_->AddObserver(this);
}

SessionStatsImpl::~SessionStatsImpl() {
  classification_runner_->RemoveObserver(this);
}

std::map<GURL, long> SessionStatsImpl::GetSessionAllowedResourcesCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allowed_map_;
}

std::map<GURL, long> SessionStatsImpl::GetSessionBlockedResourcesCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return blocked_map_;
}

void SessionStatsImpl::OnRequestMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  OnMatchedInternal(match_result, subscription);
}

void SessionStatsImpl::OnPageAllowed(
    const GURL& url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  OnMatchedInternal(FilterMatchResult::kAllowRule, subscription);
}

void SessionStatsImpl::OnPopupMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const GURL& opener_url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  OnMatchedInternal(match_result, subscription);
}

void SessionStatsImpl::OnMatchedInternal(FilterMatchResult match_result,
                                         const GURL& subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!subscription.is_empty());
  if (match_result == adblock::FilterMatchResult::kBlockRule) {
    blocked_map_[subscription]++;
  } else {
    DCHECK(match_result == adblock::FilterMatchResult::kAllowRule);
    allowed_map_[subscription]++;
  }
}

}  // namespace adblock
