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

#include "components/adblock/content/browser/page_view_stats.h"

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/adblock/content/browser/eyeo_page_info.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"

namespace adblock {
namespace {

// The key name for the kTelemetryPageViewStats dict for storing the number of
// Acceptable Ads page views.
const char kAcceptableAdsStatsCountKey[] = "aa_pageviews";

// The key name for the kTelemetryPageViewStats dict for storing the number of
// Acceptable Ads page views for Blockhthrough specific allowlisting filters.
// We recognize this case by seeing that page loads script from url
// https://btloader.com/recovery?w={{page_id}}&upapi=true which is requested by
// a page only when AA and Easylist are on. And to get notification about this
// script being loaded we add a fake blocking filter (the asterisk is for our
// browser tests which are using custom port)
// `|https://btloader.com*/recovery?w=` which is overruled by another fake
// allowing filter `@@|https://btloader.com*/recovery?w=`.
const char kAcceptableAdsBlockthroughStatsCountKey[] = "aa_bt_pageviews";

// The key name for the kTelemetryPageViewStats dict for storing the number of
// allowing filter page views.
const char kAllowedStatsCountKey[] = "allowed_pageviews";

// The key name for the kTelemetryPageViewStats dict for storing the number of
// blocking filter page views.
const char kBlockedStatsCountKey[] = "blocked_pageviews";

// This stores the total number of all page views (finished navigations) rather
// than AA page views.
const char kTotalPagesStatsCountKey[] = "pageviews";

std::string_view GetReportedNameForMetric(PageViewStats::Metric metric) {
  switch (metric) {
    case PageViewStats::Metric::AcceptableAds:
      return kAcceptableAdsStatsCountKey;
    case PageViewStats::Metric::AcceptableAdsBlockThrough:
      return kAcceptableAdsBlockthroughStatsCountKey;
    case PageViewStats::Metric::Allowing:
      return kAllowedStatsCountKey;
    case PageViewStats::Metric::Blocking:
      return kBlockedStatsCountKey;
    case PageViewStats::Metric::TotalPages:
      return kTotalPagesStatsCountKey;
  }
}

base::WeakPtr<PageViewStats> g_last_used_instance;

void RegisterNavigationWithLastUsedPageViewStats(
    content::RenderFrameHost* render_frame_host) {
  if (g_last_used_instance) {
    g_last_used_instance->RegisterMainFrameNavigation(render_frame_host);
  }
}

void RegisterAcceptableAdsBlockthroughtHitWithLastUsedPageViewStats(
    content::RenderFrameHost* render_frame_host) {
  if (g_last_used_instance) {
    g_last_used_instance->RegisterAcceptableAdsBlockthroughtHit(
        render_frame_host);
  }
}

inline bool WasNavigationCommitted(PageViewStats::Metric metric,
                                   EyeoPageInfo* page_info) {
  return page_info->HasMatchedPageView(PageViewStats::Metric::TotalPages);
}

inline bool IsNavigationCommittingNow(PageViewStats::Metric metric) {
  return metric == PageViewStats::Metric::TotalPages;
}

}  // namespace

PageViewStats::PageViewStats(
    ResourceClassificationRunner* classification_runner,
    PrefService* prefs)
    : classification_runner_(classification_runner), prefs_(prefs) {
  DCHECK(classification_runner_);
  DCHECK(prefs_);
  classification_runner_->AddObserver(this);
  g_last_used_instance = weak_factory_.GetWeakPtr();
}

PageViewStats::~PageViewStats() {
  classification_runner_->RemoveObserver(this);
}

void PageViewStats::OnRequestMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  OnMatchedInternal(url, match_result, content_type, render_frame_host,
                    subscription);
}

void PageViewStats::OnPopupMatched(const GURL& url,
                                   FilterMatchResult match_result,
                                   const GURL& opener_url,
                                   content::RenderFrameHost* render_frame_host,
                                   const GURL& subscription,
                                   const std::string& configuration_name) {
  OnMatchedInternal(url, match_result, ContentType::Other, render_frame_host,
                    subscription);
}

void PageViewStats::OnMatchedInternal(
    const GURL& url,
    FilterMatchResult match_result,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription) {
  if (render_frame_host == nullptr) {
    return;
  }

  if (match_result == FilterMatchResult::kAllowRule) {
    if (subscription == AcceptableAdsUrl()) {
      RecordPageView(render_frame_host->GetPage(), Metric::AcceptableAds);
    }
    // We increment general (any) allowlisting metric despite incrementing
    // specific, AA related metrics.
    RecordPageView(render_frame_host->GetPage(), Metric::Allowing);
  } else {
    DCHECK(match_result == FilterMatchResult::kBlockRule);
    RecordPageView(render_frame_host->GetPage(), Metric::Blocking);
  }
}

base::Value::Dict PageViewStats::GetPayload() const {
  base::Value::Dict payload;
  payload.Set(kAcceptableAdsStatsCountKey,
              GetPageViewsCount(Metric::AcceptableAds));
  payload.Set(kAcceptableAdsBlockthroughStatsCountKey,
              GetPageViewsCount(Metric::AcceptableAdsBlockThrough));
  payload.Set(kAllowedStatsCountKey, GetPageViewsCount(Metric::Allowing));
  payload.Set(kBlockedStatsCountKey, GetPageViewsCount(Metric::Blocking));
  payload.Set(kTotalPagesStatsCountKey, GetPageViewsCount(Metric::TotalPages));
  return payload;
}

void PageViewStats::ResetStats() {
  ResetPageViewsCount(Metric::AcceptableAds);
  ResetPageViewsCount(Metric::AcceptableAdsBlockThrough);
  ResetPageViewsCount(Metric::Allowing);
  ResetPageViewsCount(Metric::Blocking);
  ResetPageViewsCount(Metric::TotalPages);
}

void PageViewStats::RegisterMainFrameNavigation(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == nullptr) {
    return;
  }

  RecordPageView(render_frame_host->GetPage(), Metric::TotalPages);
}

void PageViewStats::RegisterAcceptableAdsBlockthroughtHit(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == nullptr) {
    return;
  }

  RecordPageView(render_frame_host->GetPage(),
                 Metric::AcceptableAdsBlockThrough);
}

void PageViewStats::ParkMetric(content::Page& page, Metric metric) {
  auto main_frame_id = page.GetMainDocument().GetGlobalId();
  parked_metrics_before_main_navigation_[main_frame_id].insert(metric);
}

void PageViewStats::RecordParkedMetrics(content::Page& page) {
  auto main_frame_id = page.GetMainDocument().GetGlobalId();
  for (auto it = parked_metrics_before_main_navigation_.cbegin();
       it != parked_metrics_before_main_navigation_.cend();) {
    auto* rfh = content::RenderFrameHost::FromID(it->first);
    // Check if the parked entry represents a dead RFH, if so erase it
    if (!rfh) {
      it = parked_metrics_before_main_navigation_.erase(it);
      continue;
    }
    // If this is the entry matching Page we are looking for...
    if (main_frame_id == it->first) {
      // ...and it contains some parked metrics...
      if (!it->second.empty()) {
        //...then record them
        auto* page_info = EyeoPageInfo::GetOrCreateForPage(page);
        ScopedDictPrefUpdate update(prefs_,
                                    common::prefs::kTelemetryPageViewStats);
        for (auto parked_metric : it->second) {
          auto parked_metric_key = GetReportedNameForMetric(parked_metric);
          const auto current_count_for_parked =
              update->FindInt(parked_metric_key);
          update->Set(parked_metric_key,
                      current_count_for_parked.value_or(0) + 1);
          // Now with "final" EyeoPageInfo we can mark metric as recorded
          page_info->SetMatchedPageView(parked_metric);
        }
      }
      it = parked_metrics_before_main_navigation_.erase(it);
    } else {
      ++it;
    }
  }
}

void PageViewStats::RecordPageView(content::Page& page, Metric metric) {
  auto dict_child_key = GetReportedNameForMetric(metric);
  auto* page_info = EyeoPageInfo::GetOrCreateForPage(page);
  if (!IsNavigationCommittingNow(metric) &&
      !WasNavigationCommitted(metric, page_info)) {
    ParkMetric(page, metric);
    return;
  }
  // We don't count stats metrics for individual requests but for a whole page.
  // If this is the first request matched a metric for this page, we increment
  // the counter. We store previous matches in EyeoPageInfo, so we can check if
  // this is the first metric match for this page.
  if (!page_info->SetMatchedPageView(metric)) {
    // metric was already counted
    return;
  }
  ScopedDictPrefUpdate update(prefs_, common::prefs::kTelemetryPageViewStats);
  const auto current_count = update->FindInt(dict_child_key);
  update->Set(dict_child_key, current_count.value_or(0) + 1);
  // Check parked metrics and record
  if (WasNavigationCommitted(metric, page_info)) {
    RecordParkedMetrics(page);
  }
}

int PageViewStats::GetPageViewsCount(Metric metric) const {
  auto dict_child_key = GetReportedNameForMetric(metric);
  const base::Value::Dict& dict =
      prefs_->GetDict(common::prefs::kTelemetryPageViewStats);
  const auto current_count = dict.FindInt(dict_child_key);
  return current_count.value_or(0);
}

void PageViewStats::ResetPageViewsCount(Metric metric) {
  auto dict_child_key = GetReportedNameForMetric(metric);
  ScopedDictPrefUpdate update(prefs_, common::prefs::kTelemetryPageViewStats);
  update->Set(dict_child_key, 0);
}

base::RepeatingCallback<void(content::RenderFrameHost*)>
CountNavigationsCallback() {
  return base::BindRepeating(&RegisterNavigationWithLastUsedPageViewStats);
}

base::RepeatingCallback<void(content::RenderFrameHost*)>
CountAcceptableAdsBlockthrougCallback() {
  return base::BindRepeating(
      &RegisterAcceptableAdsBlockthroughtHitWithLastUsedPageViewStats);
}

}  // namespace adblock
