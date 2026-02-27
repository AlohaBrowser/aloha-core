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
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

struct ExpectedPageViewCount {
  int aa_count;
  int aa_bt_count;
  int allowing_count;
  int blocking_count;
  int total_count;
};

class AdblockPageViewStatsTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    RenderViewHostTestHarness::SetUp();
    // The testee needs a pref for persistently storing the stats. It's
    // registered by the main pref-registering function.
    adblock::common::prefs::RegisterProfilePrefs(prefs_.registry());
    page_view_stats_ =
        std::make_unique<PageViewStats>(&classification_runner_, &prefs_);
    // Ensure that the observer was added by the constructor.
    EXPECT_TRUE(
        classification_runner_.observers_.HasObserver(page_view_stats_.get()));
  }

  void TearDown() override {
    page_view_stats_.reset();
    // Ensure that the observer was removed by the destructor.
    EXPECT_TRUE(classification_runner_.observers_.empty());
    RenderViewHostTestHarness::TearDown();
  }

  void RegisterPopupFilterHitFromFilterList(GURL subscription_url,
                                            FilterMatchResult match_result) {
    page_view_stats_->OnPopupMatched(GURL("https://example.com/popup.htmml"),
                                     match_result, GURL("https://example.com/"),
                                     main_rfh(), subscription_url,
                                     kAdblockFilteringConfigurationName);
  }

  void RegisterSubresourceFilterHitFromFilterList(
      GURL subscription_url,
      FilterMatchResult match_result,
      content::RenderFrameHost* rfh) {
    page_view_stats_->OnRequestMatched(
        GURL("https://example.com/ad.jpg"), match_result,
        std::vector<GURL>{GURL("https://example.com")}, ContentType::Image, rfh,
        subscription_url, kAdblockFilteringConfigurationName);
  }

  void RegisterSubresourceFilterHitFromFilterList(
      GURL subscription_url,
      FilterMatchResult match_result = FilterMatchResult::kAllowRule) {
    RegisterSubresourceFilterHitFromFilterList(subscription_url, match_result,
                                               main_rfh());
  }

  void VerifyPageViewCount(ExpectedPageViewCount expected) {
    auto dict_payload = page_view_stats_->GetPayload();
    auto* value = dict_payload.Find("aa_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.aa_count, value->GetInt());

    value = dict_payload.Find("aa_bt_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.aa_bt_count, value->GetInt());

    value = dict_payload.Find("allowed_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.allowing_count, value->GetInt());

    value = dict_payload.Find("blocked_pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.blocking_count, value->GetInt());

    value = dict_payload.Find("pageviews");
    ASSERT_TRUE(value);
    EXPECT_EQ(expected.total_count, value->GetInt());
  }

  using PayloadCallback = base::MockOnceCallback<void(std::string)>;
  MockResourceClassificationRunner classification_runner_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<PageViewStats> page_view_stats_;
};

TEST_F(AdblockPageViewStatsTest, NoPageViewsReportedInitially) {
  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 0});
}

TEST_F(AdblockPageViewStatsTest, EasylistAllowingHitCounted) {
  const GURL subscription_url = DefaultSubscriptionUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url,
                                             FilterMatchResult::kAllowRule);

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, EasylistBlockingHitCounted) {
  const GURL subscription_url = DefaultSubscriptionUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url,
                                             FilterMatchResult::kBlockRule);

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, EasylistPopupAllowingHitCounted) {
  const GURL subscription_url = DefaultSubscriptionUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterPopupFilterHitFromFilterList(subscription_url,
                                       FilterMatchResult::kAllowRule);

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, EasylistPopupBlockingHitCounted) {
  const GURL subscription_url = DefaultSubscriptionUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterPopupFilterHitFromFilterList(subscription_url,
                                       FilterMatchResult::kBlockRule);

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, AAAllowingHitCountedInTwoMetrics) {
  const GURL subscription_url = AcceptableAdsUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url,
                                             FilterMatchResult::kAllowRule);

  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, AABlockingHitCountedInBlockingMetric) {
  const GURL subscription_url = AcceptableAdsUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url,
                                             FilterMatchResult::kBlockRule);

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 1,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, AABlockthroughAllowingFakeFilterHitCounted) {
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  page_view_stats_->RegisterAcceptableAdsBlockthroughtHit(main_rfh());

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 1,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, MultipleFilterHitsReportedAsSinglePageView) {
  const GURL subscription_url = AcceptableAdsUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  page_view_stats_->OnRequestMatched(
      GURL("https://example.com/ad1.jpg"), FilterMatchResult::kAllowRule,
      std::vector<GURL>{GURL("https://example.com")}, ContentType::Image,
      main_rfh(), subscription_url, kAdblockFilteringConfigurationName);
  page_view_stats_->OnRequestMatched(
      GURL("https://example.com/ad2.jpg"), FilterMatchResult::kAllowRule,
      std::vector<GURL>{GURL("https://example.com")}, ContentType::Image,
      main_rfh(), subscription_url, kAdblockFilteringConfigurationName);
  page_view_stats_->OnPageAllowed(GURL("https://example.com"), main_rfh(),
                                  subscription_url,
                                  kAdblockFilteringConfigurationName);

  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, ChildFrameCountsTowardsParentsPageView) {
  const GURL subscription_url = AcceptableAdsUrl();
  // There is one filter hit in the parent frame:
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // And another in an iframe, a child of main_rfh():
  content::RenderFrameHostTester* rfh_tester =
      content::RenderFrameHostTester::For(main_rfh());
  auto* child_rfh = rfh_tester->AppendChild("subframe");
  page_view_stats_->RegisterMainFrameNavigation(child_rfh);
  RegisterSubresourceFilterHitFromFilterList(
      subscription_url, FilterMatchResult::kAllowRule, child_rfh);

  // The page view should be counted only once.
  VerifyPageViewCount({.aa_count = 1,
                       .aa_bt_count = 0,
                       .allowing_count = 1,
                       .blocking_count = 0,
                       .total_count = 1});
}

TEST_F(AdblockPageViewStatsTest, NavigatingToNewPageCreatesNewPageViewStat) {
  const GURL subscription_url = AcceptableAdsUrl();
  // Filter hit in the original page:
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // Navigate to a new page
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());

  // Filter hit in the new page
  RegisterSubresourceFilterHitFromFilterList(subscription_url);

  // Now 2 page views are reported
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});
}

TEST_F(AdblockPageViewStatsTest, PageViewStoredPersistently) {
  const GURL subscription_url = AcceptableAdsUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});

  // PageViewStats dies and is recreated on subsequent browser restart.
  // The data should be persisted (in prefs).
  page_view_stats_.reset();
  page_view_stats_ =
      std::make_unique<PageViewStats>(&classification_runner_, &prefs_);

  // The page views should be restored.
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});
}

TEST_F(AdblockPageViewStatsTest, PageViewCountResetAfterSuccessfulReport) {
  const GURL subscription_url = AcceptableAdsUrl();
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  NavigateAndCommit(GURL("https://example.com/page2.html"));
  page_view_stats_->RegisterMainFrameNavigation(main_rfh());
  RegisterSubresourceFilterHitFromFilterList(subscription_url);
  VerifyPageViewCount({.aa_count = 2,
                       .aa_bt_count = 0,
                       .allowing_count = 2,
                       .blocking_count = 0,
                       .total_count = 2});

  page_view_stats_->ResetStats();

  VerifyPageViewCount({.aa_count = 0,
                       .aa_bt_count = 0,
                       .allowing_count = 0,
                       .blocking_count = 0,
                       .total_count = 0});
}

}  // namespace adblock
