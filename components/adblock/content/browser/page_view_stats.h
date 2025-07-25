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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_PAGE_VIEW_STATS_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_PAGE_VIEW_STATS_H_

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/activeping_telemetry_topic_provider.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace adblock {

// Collects anonymous statistics about the frequency of showing acceptable ads.
// Used for evaluating the effectiveness of the Acceptable Ads program.
class PageViewStats final
    : public ResourceClassificationRunner::Observer,
      public ActivepingTelemetryTopicProvider::StatsPayloadProvider {
 public:
  enum Metric {
    AcceptableAds,
    AcceptableAdsBlockThrough,
    Allowing,
    Blocking,
    TotalPages
  };

  PageViewStats(ResourceClassificationRunner* classification_runner,
                PrefService* prefs);

  ~PageViewStats() final;

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

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) final;

  // ActivepingTelemetryTopicProvider::StatsPayloadProvider:
  base::Value::Dict GetPayload() const final;
  void ResetStats() final;

  // Counts a page view, whether it's an AA page view or not. Increments the
  // total count.
  void RegisterMainFrameNavigation(content::RenderFrameHost* render_frame_host);

  void RegisterAcceptableAdsBlockthroughtHit(
      content::RenderFrameHost* render_frame_host);

 private:
  void OnMatchedInternal(const GURL& url,
                         FilterMatchResult match_result,
                         ContentType content_type,
                         content::RenderFrameHost* render_frame_host,
                         const GURL& subscription);
  // Increments count for given metric from 0 to 1 for current page, if already
  // 1 then noop.
  void RecordPageView(content::Page& page, Metric metric);
  // We need to park metric before main navigation is committed because we need
  // wait for a "final" EyeoPageInfo object. For some unknown reason for the
  // very same RFH or Page object UserData (Document or Page) is reset when
  // navigation commits. So when page starts to load subresources before
  // navigation is committed it uses one UserData object, and then after
  // navigation is comitted there is a new empty UserData created. And by that
  // we lose tracking which metrics were already counted and count too much for
  // the same page. So when main navigation is commited we can go though parked
  // metrics and record them.
  void ParkMetric(content::Page& page, Metric metric);
  void RecordParkedMetrics(content::Page& page);
  int GetPageViewsCount(Metric metric) const;
  void ResetPageViewsCount(Metric metric);

  raw_ptr<ResourceClassificationRunner> classification_runner_;
  raw_ptr<PrefService> prefs_;
  const GURL endpoint_url_;
  std::map<content::GlobalRenderFrameHostId, std::set<Metric>>
      parked_metrics_before_main_navigation_;
  base::WeakPtrFactory<PageViewStats> weak_factory_{this};
};

// Returns a closures, calling which will increment the total page view count or
// the Acceptable Ads Blockthrough count respectively in the last used
// PageViewStats instance. NOP if there's no such instance. This is a simpler
// way to expose access to PageViewStats to external callers than to convert it
// to a KeyedService.
base::RepeatingCallback<void(content::RenderFrameHost*)>
CountNavigationsCallback();
base::RepeatingCallback<void(content::RenderFrameHost*)>
CountAcceptableAdsBlockthrougCallback();

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_PAGE_VIEW_STATS_H_
