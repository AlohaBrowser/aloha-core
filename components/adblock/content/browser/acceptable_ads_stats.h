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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ACCEPTABLE_ADS_STATS_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ACCEPTABLE_ADS_STATS_H_

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/adblock_telemetry_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace adblock {

// Collects anonymous statistics about the frequency of showing acceptable ads.
// Used for evaluating the effectiveness of the Acceptable Ads program.
class AcceptableAdsStats final : public ResourceClassificationRunner::Observer,
                                 public AdblockTelemetryService::TopicProvider {
 public:
  AcceptableAdsStats(ResourceClassificationRunner* classification_runner,
                     PrefService* prefs);

  ~AcceptableAdsStats() final;

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) final;
  // OnPageAllowed is redundant with respect to OnRequestMatched, so we can
  // ignore it.
  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) final {}
  // Popups are never Acceptable Ads, so this method is empty.
  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) final {}

  // AdblockTelemetryService::TopicProvider:
  GURL GetEndpointURL() const final;
  std::string GetAuthToken() const final;
  void GetPayload(PayloadCallback callback) const final;
  base::Time GetTimeOfNextRequest() const final;
  void ParseResponse(std::unique_ptr<std::string> response_content) final;
  void FetchDebugInfo(DebugInfoCallback callback) const final;

  // Counts a page view, whether it's an AA page view or not. Increments the
  // total count.
  void RegisterMainFrameNavigation(content::RenderFrameHost* render_frame_host);

 private:
  void IncrementAcceptableAdsPageViewsCount();
  void IncrementTotalPageViewsCount();
  int GetAcceptableAdsPageViewsCount() const;
  int GetTotalPageViewsCount() const;
  void ResetAcceptableAdsPageViewsCount();
  void ResetTotalPageViewsCount();

  raw_ptr<ResourceClassificationRunner> classification_runner_;
  raw_ptr<PrefService> prefs_;
  const GURL endpoint_url_;
  base::WeakPtrFactory<AcceptableAdsStats> weak_factory_{this};
};

// Returns a closure, calling which will increment the total page view count
// in the last used AcceptableAdsStats instance. NOP if there's no such
// instance.
// This is a simpler way to expose access to AcceptableAdsStats to
// external callers than to convert it to a KeyedService.
base::RepeatingCallback<void(content::RenderFrameHost*)>
CountNavigationsCallback();

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ACCEPTABLE_ADS_STATS_H_
