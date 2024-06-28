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

#include "components/adblock/content/browser/resource_classification_runner_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/frame_opener_info.h"
#include "components/adblock/content/browser/test/mock_frame_hierarchy_builder.h"
#include "components/adblock/core/classifier/test/mock_resource_classifier.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/test/mock_sitekey_storage.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace adblock {

using testing::_;
using testing::Return;
using Decision = ResourceClassifier::ClassificationResult::Decision;

namespace {
class MockObserver : public ResourceClassificationRunner::Observer {
 public:
  MOCK_METHOD(void,
              OnRequestMatched,
              (const GURL& url,
               FilterMatchResult match_result,
               const std::vector<GURL>& parent_frame_urls,
               ContentType content_type,
               content::RenderFrameHost* render_frame_host,
               const GURL& subscription,
               const std::string& configuration_name),
              (override));
  MOCK_METHOD(void,
              OnPageAllowed,
              (const GURL& url,
               content::RenderFrameHost* render_frame_host,
               const GURL& subscription,
               const std::string& configuration_name),
              (override));
  MOCK_METHOD(void,
              OnPopupMatched,
              (const GURL& url,
               FilterMatchResult match_result,
               const GURL& opener_url,
               content::RenderFrameHost* render_frame_host,
               const GURL& subscription,
               const std::string& configuration_name),
              (override));
};

}  // namespace

class AdblockResourceClassificationRunnerImplTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    RenderViewHostTestHarness::SetUp();
    std::unique_ptr<MockFrameHierarchyBuilder> mock_frame_hierarchy_builder =
        std::make_unique<MockFrameHierarchyBuilder>();
    mock_frame_hierarchy_builder_ = mock_frame_hierarchy_builder.get();
    mock_snapshot_ = std::make_unique<SubscriptionService::Snapshot>();
    mock_snapshot_->push_back(std::make_unique<MockSubscriptionCollection>());
    mock_resource_classifier_ = base::MakeRefCounted<MockResourceClassifier>();
    classification_runner_ = std::make_unique<ResourceClassificationRunnerImpl>(
        mock_resource_classifier_, std::move(mock_frame_hierarchy_builder),
        &mock_sitekey_storage_);
    classification_runner_->AddObserver(&mock_observer_);
  }

  void TearDown() override {
    // Avoid dangling pointers during destruction.
    mock_frame_hierarchy_builder_ = nullptr;
    classification_runner_->RemoveObserver(&mock_observer_);
    classification_runner_.reset();
    RenderViewHostTestHarness::TearDown();
  }

  MockSubscriptionCollection& mock_subscription_collection() {
    return *static_cast<MockSubscriptionCollection*>(
        mock_snapshot_->front().get());
  }

  void SiteKeyWillBePresent(const std::vector<GURL>& frame_hierarchy,
                            SiteKey sitekey) {
    EXPECT_CALL(mock_sitekey_storage_, FindSiteKeyForAnyUrl(frame_hierarchy))
        .WillRepeatedly(
            Return(std::make_pair(frame_hierarchy.front(), sitekey)));
  }

  void FrameHierarchyWillBeBuilt() {
    EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
        .WillOnce(Return(kFrameHierarchy));
  }

  void ClassifierReturnsRequestClassification(GURL url,
                                              ContentType content_type,
                                              Decision decision) {
    EXPECT_CALL(
        *mock_resource_classifier_,
        ClassifyRequest(_, url, kFrameHierarchy, content_type, kSitekey))
        .WillOnce(testing::Return(ResourceClassifier::ClassificationResult{
            decision, kSubscriptionUrl, kConfigurationName}));
  }

  void ClassifierReturnsPopupClassification(GURL popup_url, Decision decision) {
    EXPECT_CALL(*mock_resource_classifier_,
                ClassifyPopup(_, popup_url, _, kSitekey))
        .WillOnce(testing::Return(ResourceClassifier::ClassificationResult{
            decision, kSubscriptionUrl, kConfigurationName}));
  }

  void ClassifierReturnsResponseClassification(
      GURL url,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      Decision decision) {
    EXPECT_CALL(*mock_resource_classifier_,
                ClassifyResponse(_, url, kFrameHierarchy, _, headers))
        .WillOnce(testing::Return(ResourceClassifier::ClassificationResult{
            decision, kSubscriptionUrl, kConfigurationName}));
  }

  void AdMatchedWillBeNotified(GURL url,
                               ContentType content_type,
                               FilterMatchResult result) {
    EXPECT_CALL(
        mock_observer_,
        OnRequestMatched(url, result, kFrameHierarchy, content_type, main_rfh(),
                         kSubscriptionUrl, kConfigurationName));
  }

  void PopupMatchedWillBeNotified(GURL popup_url, FilterMatchResult result) {
    EXPECT_CALL(
        mock_observer_,
        OnPopupMatched(popup_url, result, kFrameHierarchy.front(), main_rfh(),
                       kSubscriptionUrl, kConfigurationName));
  }

  MockObserver mock_observer_;
  MockSitekeyStorage mock_sitekey_storage_;
  raw_ptr<MockFrameHierarchyBuilder> mock_frame_hierarchy_builder_;
  scoped_refptr<MockResourceClassifier> mock_resource_classifier_;
  std::unique_ptr<ResourceClassificationRunnerImpl> classification_runner_;
  std::unique_ptr<SubscriptionService::Snapshot> mock_snapshot_;

  const GURL kUrl{"https://test.com/url.x"};
  const GURL kWebsocketUrl{"wss://test.com/url.x"};
  const std::vector<GURL> kFrameHierarchy{GURL{"https://test.com/"}};
  const SiteKey kSitekey{"key"};
  const GURL kSubscriptionUrl{"https://easylist.com/list.txt"};
  const std::string kConfigurationName{"test_configuration"};
};

TEST_F(AdblockResourceClassificationRunnerImplTest, CheckGurlSpecNormalizes) {
  // added as a check that gurl keeps normalizing url through spec.
  EXPECT_EQ("http://hostname.com/", GURL("http:HOSTNAME.com").spec());
  EXPECT_EQ("http://hostname.com/", GURL("http:HOSTNAME.com:80").spec());
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatch_ValidParameters) {
  FrameHierarchyWillBeBuilt();
  SiteKeyWillBePresent(kFrameHierarchy, kSitekey);

  ClassifierReturnsRequestClassification(kUrl, ContentType::Image,
                                         Decision::Blocked);

  // The final callback will be called with kBlockRule result.
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kBlockRule));

  AdMatchedWillBeNotified(kUrl, ContentType::Image,
                          FilterMatchResult::kBlockRule);

  classification_runner_->CheckRequestFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      main_rfh()->GetGlobalId(), callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatchForWebSocket_ValidParameters) {
  // For WebSocket interception, we already have a RenderFrameHost found, so
  // FrameHierarchyBuilder only has to assemble the frame hierarchy.
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(kFrameHierarchy));

  SiteKeyWillBePresent(kFrameHierarchy, kSitekey);

  ClassifierReturnsRequestClassification(kWebsocketUrl, ContentType::Websocket,
                                         Decision::Blocked);

  // The final callback will be called with kBlockRule result.
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kBlockRule));

  AdMatchedWillBeNotified(kWebsocketUrl, ContentType::Websocket,
                          FilterMatchResult::kBlockRule);

  classification_runner_->CheckRequestFilterMatchForWebSocket(
      std::move(*mock_snapshot_), kWebsocketUrl, main_rfh()->GetGlobalId(),
      callback.Get());
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatch_RenderFrameHostNotFound) {
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kNoRule));

  classification_runner_->CheckRequestFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      content::GlobalRenderFrameHostId(MSG_ROUTING_NONE, MSG_ROUTING_NONE),
      callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatch_AllowlistedMainframeInOneConfig) {
  EXPECT_CALL(mock_observer_, OnPageAllowed(kUrl, main_rfh(), kSubscriptionUrl,
                                            kConfigurationName))
      .Times(0);

  // In default config page is not allowlisted...
  EXPECT_CALL(mock_subscription_collection(),
              FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                  std::vector<GURL>(), SiteKey()))
      .WillOnce(testing::Return(absl::nullopt));

  // ...so the other config is not even asked
  auto collection = std::make_unique<MockSubscriptionCollection>();
  EXPECT_CALL(*collection,
              FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                  std::vector<GURL>(), SiteKey()))
      .Times(0);
  mock_snapshot_->push_back(std::move(collection));

  classification_runner_->CheckDocumentAllowlisted(
      std::move(*mock_snapshot_), kUrl, main_rfh()->GetGlobalId());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatch_AllowlistedMainframeInTwoConfigs) {
  std::string other_configuration = "other";
  EXPECT_CALL(mock_observer_, OnPageAllowed(kUrl, main_rfh(), kSubscriptionUrl,
                                            other_configuration));

  EXPECT_CALL(mock_subscription_collection(),
              FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                  std::vector<GURL>(), SiteKey()))
      .WillOnce(testing::Return(absl::optional<GURL>{kSubscriptionUrl}));

  auto collection = std::make_unique<MockSubscriptionCollection>();
  EXPECT_CALL(*collection,
              FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                  std::vector<GURL>(), SiteKey()))
      .WillOnce(testing::Return(absl::optional<GURL>{kSubscriptionUrl}));
  EXPECT_CALL(*collection, GetFilteringConfigurationName())
      .WillRepeatedly(testing::ReturnRef(other_configuration));
  EXPECT_CALL(mock_subscription_collection(), GetFilteringConfigurationName())
      .WillRepeatedly(testing::ReturnRef(kConfigurationName));
  mock_snapshot_->push_back(std::move(collection));

  classification_runner_->CheckDocumentAllowlisted(
      std::move(*mock_snapshot_), kUrl, main_rfh()->GetGlobalId());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckRequestFilterMatch_RenderFrameHostDiesBeforeElementHiding) {
  FrameHierarchyWillBeBuilt();
  SiteKeyWillBePresent(kFrameHierarchy, kSitekey);

  ClassifierReturnsRequestClassification(kUrl, ContentType::Image,
                                         Decision::Blocked);

  // The final callback will be called with kBlockRule result.
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kBlockRule));

  classification_runner_->CheckRequestFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,

      main_rfh()->GetGlobalId(), callback.Get());

  // Before running the thread loop, destroy the RFH.
  DeleteContents();
  // Observer will be notified with null RFH.
  EXPECT_CALL(mock_observer_,
              OnRequestMatched(kUrl, FilterMatchResult::kBlockRule,
                               kFrameHierarchy, ContentType::Image, nullptr,
                               kSubscriptionUrl, kConfigurationName))
      .Times(0);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckResponseFilterMatch_NoRenderFrameHost) {
  base::MockCallback<CheckFilterMatchCallback> callback;

  EXPECT_CALL(callback, Run(FilterMatchResult::kNoRule));

  classification_runner_->CheckResponseFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      content::GlobalRenderFrameHostId(MSG_ROUTING_NONE, MSG_ROUTING_NONE), {},
      callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckResponseFilterMatch_ResponseFilterMatchResultBlocked) {
  auto headers = net::HttpResponseHeaders::TryToCreate("whatever");
  ASSERT_TRUE(headers);
  base::MockCallback<CheckFilterMatchCallback> callback;

  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetGlobalId()))
      .WillOnce(testing::Return(main_rfh()));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(kFrameHierarchy));
  ClassifierReturnsResponseClassification(kUrl, headers, Decision::Blocked);

  classification_runner_->CheckResponseFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      main_rfh()->GetGlobalId(), headers, callback.Get());

  EXPECT_CALL(callback, Run(FilterMatchResult::kBlockRule));
  EXPECT_CALL(
      mock_observer_,
      OnRequestMatched(kUrl, FilterMatchResult::kBlockRule, kFrameHierarchy, _,
                       _, kSubscriptionUrl, kConfigurationName));
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckResponseFilterMatch_ResponseFilterMatchResultAllowed) {
  auto headers = net::HttpResponseHeaders::TryToCreate("whatever");
  ASSERT_TRUE(headers);
  base::MockCallback<CheckFilterMatchCallback> callback;

  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetGlobalId()))
      .WillOnce(testing::Return(main_rfh()));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(kFrameHierarchy));
  ClassifierReturnsResponseClassification(kUrl, headers, Decision::Allowed);

  classification_runner_->CheckResponseFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      main_rfh()->GetGlobalId(), headers, callback.Get());

  EXPECT_CALL(callback, Run(FilterMatchResult::kAllowRule));
  EXPECT_CALL(
      mock_observer_,
      OnRequestMatched(kUrl, FilterMatchResult::kAllowRule, kFrameHierarchy, _,
                       _, kSubscriptionUrl, kConfigurationName));
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckResponseFilterMatch_ResponseFilterMatchResultIgnored) {
  auto headers = net::HttpResponseHeaders::TryToCreate("whatever");
  ASSERT_TRUE(headers);
  base::MockCallback<CheckFilterMatchCallback> callback;

  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetGlobalId()))
      .WillOnce(testing::Return(main_rfh()));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(kFrameHierarchy));
  ClassifierReturnsResponseClassification(kUrl, headers, Decision::Ignored);

  classification_runner_->CheckResponseFilterMatch(
      std::move(*mock_snapshot_), kUrl, ContentType::Image,
      main_rfh()->GetGlobalId(), headers, callback.Get());

  EXPECT_CALL(callback, Run(FilterMatchResult::kNoRule));
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckPopupFilterMatch_ValidParameters) {
  // Set opener pointing to valid RFH
  adblock::FrameOpenerInfo::CreateForWebContents(web_contents());
  auto* info = adblock::FrameOpenerInfo::FromWebContents(web_contents());
  info->SetOpener(main_rfh()->GetGlobalId());

  FrameHierarchyWillBeBuilt();
  SiteKeyWillBePresent(kFrameHierarchy, kSitekey);

  ClassifierReturnsPopupClassification(kUrl, Decision::Blocked);

  // The final callback will be called with kBlockRule result.
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kBlockRule));

  PopupMatchedWillBeNotified(kUrl, FilterMatchResult::kBlockRule);

  classification_runner_->CheckPopupFilterMatch(std::move(*mock_snapshot_),
                                                kUrl, main_rfh()->GetGlobalId(),
                                                callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckPopupFilterMatch_RenderFrameHostNotFound) {
  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kNoRule));

  classification_runner_->CheckPopupFilterMatch(
      std::move(*mock_snapshot_), kUrl,
      content::GlobalRenderFrameHostId(MSG_ROUTING_NONE, MSG_ROUTING_NONE),
      callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockResourceClassificationRunnerImplTest,
       CheckPopupFilterMatch_RenderFrameHostNotFoundForOpener) {
  // Set opener pointing to invalid RFH
  adblock::FrameOpenerInfo::CreateForWebContents(web_contents());

  base::MockCallback<CheckFilterMatchCallback> callback;
  EXPECT_CALL(callback, Run(FilterMatchResult::kNoRule));

  classification_runner_->CheckPopupFilterMatch(std::move(*mock_snapshot_),
                                                kUrl, main_rfh()->GetGlobalId(),
                                                callback.Get());

  task_environment()->RunUntilIdle();
}

}  // namespace adblock
