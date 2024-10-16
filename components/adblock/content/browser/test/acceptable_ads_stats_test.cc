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

#include <memory>
#include <string>
#include <vector>

#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

struct ExpectedPageViewCount {
  int aa_count;
  int total_count;
};

class AdblockAcceptableAdsStatsTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    RenderViewHostTestHarness::SetUp();
    // The testee needs a pref for persistently storing the stats. It's
    // registered by the main pref-registering function.
    adblock::common::prefs::RegisterProfilePrefs(prefs_.registry());
    acceptable_ads_stats_ =
        std::make_unique<AcceptableAdsStats>(&classification_runner_, &prefs_);
    // Ensure that the observer was added by the constructor.
    EXPECT_TRUE(classification_runner_.observers_.HasObserver(
        acceptable_ads_stats_.get()));
  }

  void TearDown() override {
    acceptable_ads_stats_.reset();
    // Ensure that the observer was removed by the destructor.
    EXPECT_TRUE(classification_runner_.observers_.empty());
    RenderViewHostTestHarness::TearDown();
  }

  void RegisterSubresourceFilterHitFromFilterList(
      GURL subscription_url,
      content::RenderFrameHost* rfh) {
    acceptable_ads_stats_->OnRequestMatched(
        GURL("https://example.com/ad.jpg"), FilterMatchResult::kAllowRule,
        std::vector<GURL>{GURL("https://example.com")}, ContentType::Image, rfh,
        subscription_url, kAdblockFilteringConfigurationName);
  }

  void RegisterSubresourceFilterHitFromFilterList(GURL subscription_url) {
    RegisterSubresourceFilterHitFromFilterList(subscription_url, main_rfh());
  }

  void VerifyPageViewCount(ExpectedPageViewCount expected) {
    // TODO(mpawlowski): the format of this payload will change with DPD-2794.
    // For now it's just a NumberToString of the AA page views.
    PayloadCallback payload_callback;
    EXPECT_CALL(payload_callback,
                Run(base::NumberToString(expected.aa_count) + " " +
                    base::NumberToString(expected.total_count)));
    acceptable_ads_stats_->GetPayload(payload_callback.Get());
  }

  using PayloadCallback = base::MockOnceCallback<void(std::string)>;
  MockResourceClassificationRunner classification_runner_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<AcceptableAdsStats> acceptable_ads_stats_;
};

TEST_F(AdblockAcceptableAdsStatsTest, NoPageViewsReportedInitially) {
  VerifyPageViewCount({.aa_count = 0, .total_count = 0});
}

TEST_F(AdblockAcceptableAdsStatsTest, EasylistHitDoesNotCount) {
  const GURL subscription_url = DefaultSubscriptionUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // Only total page view count is incremented, not the AA count.
  VerifyPageViewCount({.aa_count = 0, .total_count = 1});
}

TEST_F(AdblockAcceptableAdsStatsTest,
       PageViewReportedWhenSubresourceFilterHit) {
  const GURL subscription_url = AcceptableAdsUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

TEST_F(AdblockAcceptableAdsStatsTest,
       MultipleFilterHitsReportedAsSinglePageView) {
  const GURL subscription_url = AcceptableAdsUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  acceptable_ads_stats_->OnRequestMatched(
      GURL("https://example.com/ad1.jpg"), FilterMatchResult::kAllowRule,
      std::vector<GURL>{GURL("https://example.com")}, ContentType::Image,
      main_rfh(), subscription_url, kAdblockFilteringConfigurationName);
  acceptable_ads_stats_->OnRequestMatched(
      GURL("https://example.com/ad2.jpg"), FilterMatchResult::kAllowRule,
      std::vector<GURL>{GURL("https://example.com")}, ContentType::Image,
      main_rfh(), subscription_url, kAdblockFilteringConfigurationName);
  acceptable_ads_stats_->OnPageAllowed(GURL("https://example.com"), main_rfh(),
                                       subscription_url,
                                       kAdblockFilteringConfigurationName);

  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

TEST_F(AdblockAcceptableAdsStatsTest, BlockingStillCountsAsAAHit) {
  // The AA list contains some blocking filters, not just allowing filters.
  // A match of a blocking filter from the AA filter list also counts as an AA
  // page view.
  const GURL subscription_url = AcceptableAdsUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  acceptable_ads_stats_->OnRequestMatched(
      GURL("https://example.com/ad.jpg"), FilterMatchResult::kBlockRule,
      std::vector<GURL>{GURL("https://example.com")}, ContentType::Image,
      main_rfh(), subscription_url, kAdblockFilteringConfigurationName);

  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

TEST_F(AdblockAcceptableAdsStatsTest, ChildFrameCountsTowardsParentsPageView) {
  const GURL subscription_url = AcceptableAdsUrl();
  // There is one filter hit in the parent frame:
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // And another in an iframe, a child of main_rfh():
  content::RenderFrameHostTester* rfh_tester =
      content::RenderFrameHostTester::For(main_rfh());
  auto* child_rfh = rfh_tester->AppendChild("subframe");
  acceptable_ads_stats_->RegisterMainFrameNavigation(child_rfh);
  RegisterSubresourceFilterHitFromFilterList(subscription_url, child_rfh);

  // The page view should be counted only once.
  VerifyPageViewCount({.aa_count = 1, .total_count = 1});
}

TEST_F(AdblockAcceptableAdsStatsTest,
       NavigatingToNewPageCreatesNewPageViewStat) {
  const GURL subscription_url = AcceptableAdsUrl();
  // Filter hit in the original page:
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // Navigate to a new page
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());

  // Filter hit in the new page
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // Now 2 page views are reported
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

TEST_F(AdblockAcceptableAdsStatsTest, PageViewStoredPersistently) {
  const GURL subscription_url = AcceptableAdsUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});

  // AcceptableAdsStats dies and is recreated on subsequent browser restart.
  // The data should be persisted (in prefs).
  acceptable_ads_stats_.reset();
  acceptable_ads_stats_ =
      std::make_unique<AcceptableAdsStats>(&classification_runner_, &prefs_);

  // The page views should be restored.
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});
}

TEST_F(AdblockAcceptableAdsStatsTest, PageViewCountResetAfterSuccessfulReport) {
  const GURL subscription_url = AcceptableAdsUrl();
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  acceptable_ads_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  VerifyPageViewCount({.aa_count = 2, .total_count = 2});

  acceptable_ads_stats_->ParseResponse(std::make_unique<std::string>("OK"));

  VerifyPageViewCount({.aa_count = 0, .total_count = 0});
}

}  // namespace adblock
