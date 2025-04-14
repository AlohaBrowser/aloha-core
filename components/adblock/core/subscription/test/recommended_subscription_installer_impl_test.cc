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

#include "components/adblock/core/subscription/recommended_subscription_installer_impl.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/configuration/test/mock_filtering_configuration.h"
#include "components/adblock/core/net/test/mock_adblock_resource_request.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockRecommendedSubscriptionInstallerImplTest : public testing::Test {
 public:
  void SetUp() override {
    common::prefs::RegisterProfilePrefs(prefs_.registry());

    // Only adblock filtering configurations can have recommended subscriptions
    // installed
    const std::string adblock = "adblock";
    EXPECT_CALL(filtering_configuration_, GetName())
        .WillRepeatedly(testing::ReturnRef(adblock));

    // Create tmp dir to store mock downloaded recommendation list.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    recommended_subscription_installer_ =
        std::make_unique<RecommendedSubscriptionInstallerImpl>(
            &prefs_, &filtering_configuration_, &persistent_metadata_,
            request_maker_.Get());
  }

  void MockDownloadedFile(const std::string& content) {
    ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.GetPath(),
                                               &recommendation_json_file_));
    ASSERT_TRUE(base::WriteFile(recommendation_json_file_, content));
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath recommendation_json_file_;
  TestingPrefServiceSimple prefs_;
  MockFilteringConfiguration filtering_configuration_;
  MockSubscriptionPersistentMetadata persistent_metadata_;
  base::MockCallback<RecommendedSubscriptionInstallerImpl::ResourceRequestMaker>
      request_maker_;
  std::unique_ptr<RecommendedSubscriptionInstallerImpl>
      recommended_subscription_installer_;
};

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       UpdateIsDueOnFirstStart) {
  // Next time recommended subscriptions should be updated is set to
  // base::Time::Now() on the first start.
  ASSERT_TRUE(prefs_.GetTime(
                  common::prefs::kAutoInstalledSubscriptionsNextUpdateTime) <=
              base::Time::Now());

  // ResourceRequestMaker is asked to create a request ...
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    // ... and recommended subscription list download starts.
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(RecommendedSubscriptionListUrl(),
              AdblockResourceRequest::Method::GET, testing::_,
              AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, ""));
    return mock_ongoing_request;
  });

  recommended_subscription_installer_->RunUpdateCheck();
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       NoRequestMadeIfUpdateIsNotDue) {
  // Set the next time recommended subscriptions should be updated to a time
  // later then now.
  prefs_.SetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime,
                 base::Time::Now() + base::Days(1));

  // Since the update is not due no request is made.
  EXPECT_CALL(request_maker_, Run()).Times(0);

  recommended_subscription_installer_->RunUpdateCheck();
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       NoRecommendedSubscriptionsReceivedMeansNoInstalls) {
  // ResourceRequestMaker is asked to create a request ...
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    // ... and recommended subscription list download starts.
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(RecommendedSubscriptionListUrl(),
              AdblockResourceRequest::Method::GET, testing::_,
              AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, ""))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  recommended_subscription_installer_->RunUpdateCheck();

  // Recommended subscriptions is an empty file.
  MockDownloadedFile("");

  // Store next update time before running the update so we can check if it got
  // updated.
  auto kAutoInstalledSubscriptionsNextUpdateTimeBefore =
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime);

  // OnRecommendationListDownloaded called with empty file.
  response_callback.Run(RecommendedSubscriptionListUrl(),
                        recommendation_json_file_, nullptr);

  // No subscriptions should installed.
  EXPECT_CALL(filtering_configuration_, AddFilterList(testing::_)).Times(0);
  // No expiration times should updated.
  EXPECT_CALL(persistent_metadata_,
              SetAutoInstalledExpirationInterval(testing::_, testing::_))
      .Times(0);

  task_environment_.RunUntilIdle();

  // kAutoInstalledSubscriptionsNextUpdateTime got updated.
  EXPECT_TRUE(
      kAutoInstalledSubscriptionsNextUpdateTimeBefore <
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime));
  EXPECT_FALSE(base::PathExists(recommendation_json_file_));
}

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       RecommendedSubscriptionsGetInstalledOrUpdated) {
  // ResourceRequestMaker is asked to create a request ...
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    // ... and recommended subscription list download starts.
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(RecommendedSubscriptionListUrl(),
              AdblockResourceRequest::Method::GET, testing::_,
              AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, ""))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  recommended_subscription_installer_->RunUpdateCheck();

  // Expect that recommended subscriptions get installed.
  EXPECT_CALL(filtering_configuration_,
              AddFilterList(GURL("https://recommended-fl/list1.txt")));
  EXPECT_CALL(filtering_configuration_,
              AddFilterList(GURL("https://recommended-fl/list2.txt")));

  // Expect that recommended subscriptions expiration times get updated.
  EXPECT_CALL(persistent_metadata_,
              SetAutoInstalledExpirationInterval(
                  GURL("https://recommended-fl/list1.txt"), base::Days(14)));
  EXPECT_CALL(persistent_metadata_,
              SetAutoInstalledExpirationInterval(
                  GURL("https://recommended-fl/list2.txt"), base::Days(14)));

  // list1 was already auto installed.
  EXPECT_CALL(filtering_configuration_,
              IsFilterListPresent(GURL("https://recommended-fl/list1.txt")))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(persistent_metadata_,
              IsAutoInstalled(GURL("https://recommended-fl/list1.txt")))
      .WillOnce(testing::Return(true));

  // list2 was not yet installed.
  EXPECT_CALL(filtering_configuration_,
              IsFilterListPresent(GURL("https://recommended-fl/list2.txt")))
      .WillOnce(testing::Return(false));

  MockDownloadedFile(
      "[{\"url\": \"https://recommended-fl/list1.txt\"}, {\"url\": "
      "\"https://recommended-fl/list2.txt\"}]");

  // Store next update time before running the update so we can check if it got
  // updated.
  auto kAutoInstalledSubscriptionsNextUpdateTimeBefore =
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime);

  // OnRecommendationListDownloaded called with file containing a list of
  // recommended subscription urls.
  response_callback.Run(RecommendedSubscriptionListUrl(),
                        recommendation_json_file_, nullptr);

  task_environment_.RunUntilIdle();

  // kAutoInstalledSubscriptionsNextUpdateTime got updated.
  EXPECT_TRUE(
      kAutoInstalledSubscriptionsNextUpdateTimeBefore <
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime));
  EXPECT_FALSE(base::PathExists(recommendation_json_file_));
}

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       SkipAlreadyAddedNotAutoInstalledRecommendedSubscription) {
  // ResourceRequestMaker is asked to create a request ...
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    // ... and recommended subscription list download starts.
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(RecommendedSubscriptionListUrl(),
              AdblockResourceRequest::Method::GET, testing::_,
              AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, ""))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  recommended_subscription_installer_->RunUpdateCheck();

  // No subscriptions should installed.
  EXPECT_CALL(filtering_configuration_, AddFilterList(testing::_)).Times(0);
  // No expiration times should updated.
  EXPECT_CALL(persistent_metadata_,
              SetAutoInstalledExpirationInterval(testing::_, testing::_))
      .Times(0);

  // list1 was already installed.
  EXPECT_CALL(filtering_configuration_,
              IsFilterListPresent(GURL("https://recommended-fl/list1.txt")))
      .WillOnce(testing::Return(true));
  // list1 was not auto installed but added by user / device locale.
  EXPECT_CALL(persistent_metadata_,
              IsAutoInstalled(GURL("https://recommended-fl/list1.txt")))
      .WillOnce(testing::Return(false));

  MockDownloadedFile("[{\"url\": \"https://recommended-fl/list1.txt\"}]");

  // Store next update time before running the update so we can check if it got
  // updated.
  auto kAutoInstalledSubscriptionsNextUpdateTimeBefore =
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime);

  // OnRecommendationListDownloaded called with file containing a list of
  // recommended subscription urls
  response_callback.Run(RecommendedSubscriptionListUrl(),
                        recommendation_json_file_, nullptr);

  task_environment_.RunUntilIdle();

  // kAutoInstalledSubscriptionsNextUpdateTime got updated.
  EXPECT_TRUE(
      kAutoInstalledSubscriptionsNextUpdateTimeBefore <
      prefs_.GetTime(common::prefs::kAutoInstalledSubscriptionsNextUpdateTime));
  EXPECT_FALSE(base::PathExists(recommendation_json_file_));
}

TEST_F(AdblockRecommendedSubscriptionInstallerImplTest,
       ExpiredAutoInstalledSubscriptionGetsRemoved) {
  // ResourceRequestMaker is asked to create a request ...
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    // ... and recommended subscription list download starts.
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(RecommendedSubscriptionListUrl(),
              AdblockResourceRequest::Method::GET, testing::_,
              AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded, ""))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  const GURL list1("https://recommended-fl/list1.txt");
  const GURL list2("https://recommended-fl/list2.txt");
  const std::vector<GURL> installed_subcriptions = {list1, list2};

  EXPECT_CALL(filtering_configuration_, GetFilterLists())
      .WillOnce(testing::Return(installed_subcriptions));

  // list1.txt is not yet expired.
  EXPECT_CALL(persistent_metadata_, IsAutoInstalledExpired(list1))
      .WillOnce(testing::Return(false));
  // list2.txt was not recommended in a while and will be removed.
  EXPECT_CALL(persistent_metadata_, IsAutoInstalledExpired(list2))
      .WillOnce(testing::Return(true));

  EXPECT_CALL(filtering_configuration_, RemoveFilterList(list2));

  recommended_subscription_installer_->RunUpdateCheck();

  // Expired auto installed lists get removed even when parsing fails.
  MockDownloadedFile("[]");

  response_callback.Run(RecommendedSubscriptionListUrl(),
                        recommendation_json_file_, nullptr);

  task_environment_.RunUntilIdle();
  EXPECT_FALSE(base::PathExists(recommendation_json_file_));
}

}  // namespace adblock
