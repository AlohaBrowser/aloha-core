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

#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

namespace {

constexpr char kAllowedTestSub[] = "http://allowed.sub.com/";
constexpr char kBlockedTestSub[] = "http://blocked.sub.com/";

}  // namespace

class AdblockSessionStatsTest : public testing::Test {
 public:
  AdblockSessionStatsTest() = default;

  void SetUp() override {
    session_stats_ = std::make_unique<SessionStatsImpl>(&classfier_);
  }

  MockResourceClassificationRunner classfier_;
  std::unique_ptr<SessionStats> session_stats_;
};

TEST_F(AdblockSessionStatsTest, UrlStatsDataCollected) {
  classfier_.NotifyResourceMatched(
      GURL(), FilterMatchResult::kAllowRule, std::vector<GURL>(),
      ContentType::Subdocument, nullptr, GURL{kAllowedTestSub});

  classfier_.NotifyResourceMatched(
      GURL(), FilterMatchResult::kBlockRule, std::vector<GURL>(),
      ContentType::Subdocument, nullptr, GURL{kBlockedTestSub});

  auto allowed_count = session_stats_->GetSessionAllowedResourcesCount();
  auto blocked_count = session_stats_->GetSessionBlockedResourcesCount();

  EXPECT_EQ((std::map<GURL, long>{{GURL(kAllowedTestSub), 1}}), allowed_count);
  EXPECT_EQ((std::map<GURL, long>{{GURL(kBlockedTestSub), 1}}), blocked_count);

  classfier_.NotifyResourceMatched(
      GURL(), FilterMatchResult::kBlockRule, std::vector<GURL>(),
      ContentType::Subdocument, nullptr, GURL{kBlockedTestSub});

  allowed_count = session_stats_->GetSessionAllowedResourcesCount();
  blocked_count = session_stats_->GetSessionBlockedResourcesCount();

  EXPECT_EQ((std::map<GURL, long>{{GURL(kAllowedTestSub), 1}}), allowed_count);
  EXPECT_EQ((std::map<GURL, long>{{GURL(kBlockedTestSub), 2}}), blocked_count);
}

TEST_F(AdblockSessionStatsTest, EheStatsDataCollected) {
  EXPECT_EQ(session_stats_->GetHiddenResourcesCount().size(), 0u);

  classfier_.NotifyPageElementMatched("hidden", ElementHideAction::kHide);

  classfier_.NotifyPageElementMatched("removed", ElementHideAction::kRemove);

  classfier_.NotifyPageElementMatched("css", ElementHideAction::kInlineCss);

  auto eh_count = session_stats_->GetHiddenResourcesCount();
  EXPECT_EQ(eh_count.size(), 3u);
  EXPECT_EQ(eh_count[ElementHideAction::kHide], 1L);
  EXPECT_EQ(eh_count[ElementHideAction::kRemove], 1L);
  EXPECT_EQ(eh_count[ElementHideAction::kInlineCss], 1L);
}

}  // namespace adblock
