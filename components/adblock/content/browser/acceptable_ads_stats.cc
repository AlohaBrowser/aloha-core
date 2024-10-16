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

#include "components/adblock/content/browser/acceptable_ads_stats.h"

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/adblock/content/browser/eyeo_document_info.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"

namespace adblock {
namespace {

// The key name for the kAcceptableAdsStats dict for storing the number of
// Acceptable Ads page views.
const char kAcceptableAdsStatsCountKey[] = "acceptable_ads_page_views_count";

// This stores the total number of all page views (finished navigations) rather
// than AA page views.
const char kTotalStatsCountKey[] = "total_page_views_count";

base::WeakPtr<AcceptableAdsStats> g_last_used_instance;

void RegisterNavigationWithLastUsedAcceptableAdsStats(
    content::RenderFrameHost* render_frame_host) {
  if (g_last_used_instance) {
    g_last_used_instance->RegisterMainFrameNavigation(render_frame_host);
  }
}

}  // namespace

AcceptableAdsStats::AcceptableAdsStats(
    ResourceClassificationRunner* classification_runner,
    PrefService* prefs)
    : classification_runner_(classification_runner), prefs_(prefs) {
  DCHECK(classification_runner_);
  DCHECK(prefs_);
  classification_runner_->AddObserver(this);
  g_last_used_instance = weak_factory_.GetWeakPtr();
}

AcceptableAdsStats::~AcceptableAdsStats() {
  classification_runner_->RemoveObserver(this);
}

void AcceptableAdsStats::OnRequestMatched(
    const GURL& url,
    FilterMatchResult match_result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  if (subscription != AcceptableAdsUrl()) {
    return;
  }
  if (render_frame_host == nullptr) {
    return;
  }

  // We only count page views, not individual requests. If this is the first
  // request matched for this page, increment the counter. We store previous
  // matches in EyeoDocumentInfo, so we can check if this is the first request
  // for this page.
  content::RenderFrameHost* main_frame =
      render_frame_host->GetOutermostMainFrame();
  EyeoDocumentInfo* document_info =
      EyeoDocumentInfo::GetOrCreateForCurrentDocument(main_frame);
  if (document_info->HasMatchedAcceptableAds()) {
    return;
  }
  document_info->SetMatchedAcceptableAds();
  IncrementAcceptableAdsPageViewsCount();
}

GURL AcceptableAdsStats::GetEndpointURL() const {
  // TODO(mpawlowski): implement actual endpoint in DPD-2794
  return GURL();
}

std::string AcceptableAdsStats::GetAuthToken() const {
  // TODO(mpawlowski): implement actual auth token in DPD-2794
  return std::string();
}

void AcceptableAdsStats::GetPayload(PayloadCallback callback) const {
  // TODO(mpawlowski): the actual payload will be more complex when DPD-2794 is
  // implemented. This is just for testing purposes.
  const std::string payload =
      base::NumberToString(GetAcceptableAdsPageViewsCount()) + " " +
      base::NumberToString(GetTotalPageViewsCount());
  std::move(callback).Run(std::move(payload));
}

base::Time AcceptableAdsStats::GetTimeOfNextRequest() const {
  // TODO(mpawlowski): implement actual time of next request in DPD-2794
  return base::Time::Max();
}

void AcceptableAdsStats::ParseResponse(
    std::unique_ptr<std::string> response_content) {
  // TODO(mpawlowski): implement actual response parsing in DPD-2794
  // For now, assume the response is "success" when not null, which lets us
  // reset the counter. The next request will have the number of page views
  // since the last successful request.
  if (response_content) {
    ResetAcceptableAdsPageViewsCount();
    ResetTotalPageViewsCount();
  }
}

void AcceptableAdsStats::FetchDebugInfo(DebugInfoCallback callback) const {
  const std::string debug_info =
      "Acceptable Ads page views since last request: " +
      base::NumberToString(GetAcceptableAdsPageViewsCount()) + "\n" +
      "Total page views since last request: " +
      base::NumberToString(GetTotalPageViewsCount());
  std::move(callback).Run(std::move(debug_info));
}

void AcceptableAdsStats::RegisterMainFrameNavigation(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == nullptr) {
    return;
  }

  // We only count page views, not individual requests. If this is the first
  // request matched for this page, increment the counter. We store previous
  // matches in EyeoDocumentInfo, so we can check if this is the first request
  // for this page.
  content::RenderFrameHost* main_frame =
      render_frame_host->GetOutermostMainFrame();
  EyeoDocumentInfo* document_info =
      EyeoDocumentInfo::GetOrCreateForCurrentDocument(main_frame);
  if (document_info->IsNavigationCounted()) {
    return;
  }
  document_info->SetNavigationCounted();
  IncrementTotalPageViewsCount();
}

void AcceptableAdsStats::IncrementAcceptableAdsPageViewsCount() {
  ScopedDictPrefUpdate update(prefs_,
                              common::prefs::kTelemetryAcceptableAdsStats);
  const auto current_count = update->FindInt(kAcceptableAdsStatsCountKey);
  update->Set(kAcceptableAdsStatsCountKey, current_count.value_or(0) + 1);
}

void AcceptableAdsStats::IncrementTotalPageViewsCount() {
  ScopedDictPrefUpdate update(prefs_,
                              common::prefs::kTelemetryAcceptableAdsStats);
  const auto current_count = update->FindInt(kTotalStatsCountKey);
  update->Set(kTotalStatsCountKey, current_count.value_or(0) + 1);
}

int AcceptableAdsStats::GetAcceptableAdsPageViewsCount() const {
  const base::Value::Dict& dict =
      prefs_->GetDict(common::prefs::kTelemetryAcceptableAdsStats);
  const auto current_count = dict.FindInt(kAcceptableAdsStatsCountKey);
  return current_count.value_or(0);
}

int AcceptableAdsStats::GetTotalPageViewsCount() const {
  const base::Value::Dict& dict =
      prefs_->GetDict(common::prefs::kTelemetryAcceptableAdsStats);
  const auto current_count = dict.FindInt(kTotalStatsCountKey);
  return current_count.value_or(0);
}

void AcceptableAdsStats::ResetAcceptableAdsPageViewsCount() {
  ScopedDictPrefUpdate update(prefs_,
                              common::prefs::kTelemetryAcceptableAdsStats);
  update->Set(kAcceptableAdsStatsCountKey, 0);
}

void AcceptableAdsStats::ResetTotalPageViewsCount() {
  ScopedDictPrefUpdate update(prefs_,
                              common::prefs::kTelemetryAcceptableAdsStats);
  update->Set(kTotalStatsCountKey, 0);
}

base::RepeatingCallback<void(content::RenderFrameHost*)>
CountNavigationsCallback() {
  return base::BindRepeating(&RegisterNavigationWithLastUsedAcceptableAdsStats);
}

}  // namespace adblock
