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

#include "components/adblock/content/browser/adblock_url_loader_factory.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/frame_opener_info.h"
#include "components/adblock/content/browser/test/mock_adblock_content_security_policy_injector.h"
#include "components/adblock/content/browser/test/mock_element_hider.h"
#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "components/adblock/core/test/mock_sitekey_storage.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kTestUserAgent[] = "test-user-agent";

enum class HostState { Alive, Dead };

class TestURLLoaderFactory : public adblock::AdblockURLLoaderFactory {
 public:
  TestURLLoaderFactory(
      adblock::AdblockURLLoaderFactoryConfig config,
      content::GlobalRenderFrameHostId host_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      std::string user_agent_string,
      DisconnectCallback on_disconnect)
      : adblock::AdblockURLLoaderFactory(std::move(config),
                                         host_id,
                                         std::move(receiver),
                                         std::move(target_factory),
                                         user_agent_string,
                                         std::move(on_disconnect)),
        state_(HostState::Alive) {}

  bool CheckHostValid() const override { return state_ == HostState::Alive; }
  void ApplyState(HostState state) { state_ = state; }

 private:
  HostState state_;
};

struct RequestFlow {
  GURL url{"https://test.com"};
  adblock::FilterMatchResult request_match =
      adblock::FilterMatchResult::kAllowRule;
  adblock::FilterMatchResult response_match =
      adblock::FilterMatchResult::kAllowRule;
  bool element_hidable = true;
  network::mojom::RequestDestination destination =
      network::mojom::RequestDestination::kFrame;
};

class AdblockURLLoaderFactoryTest : public content::RenderViewHostTestHarness {
 public:
  AdblockURLLoaderFactoryTest() : test_factory_receiver_(&test_factory_) {}

  AdblockURLLoaderFactoryTest(const AdblockURLLoaderFactoryTest&) = delete;
  AdblockURLLoaderFactoryTest& operator=(const AdblockURLLoaderFactoryTest&) =
      delete;

  void StartRequest() {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = flow.url;
    request->destination = flow.destination;
    ConfigureSubscriptionService();

    loader_ = network::SimpleURLLoader::Create(std::move(request),
                                               TRAFFIC_ANNOTATION_FOR_TESTS);
    mojo::Remote<network::mojom::URLLoaderFactory> factory_remote;
    auto factory_request = factory_remote.BindNewPipeAndPassReceiver();
    loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        factory_remote.get(),
        base::BindOnce(&AdblockURLLoaderFactoryTest::OnDownload,
                       base::Unretained(this)));

    adblock_factory_ = std::make_unique<TestURLLoaderFactory>(
        adblock::AdblockURLLoaderFactoryConfig{
            &subscription_service_, &resource_classifier_, &element_hider_,
            &sitekey_storage_, &csp_injector_},
        web_contents()->GetPrimaryMainFrame()->GetGlobalId(),
        std::move(factory_request),
        test_factory_receiver_.BindNewPipeAndPassRemote(), kTestUserAgent,
        base::BindOnce(&AdblockURLLoaderFactoryTest::OnDisconnect,
                       base::Unretained(this)));

    test_factory_.AddResponse(flow.url.spec(), "Hello.");
    base::RunLoop().RunUntilIdle();
  }

  void OnDownload(std::optional<std::string> response_body) {}

  void ConfigureSubscriptionService() {
    EXPECT_CALL(subscription_service_, GetCurrentSnapshot())
        .WillRepeatedly([]() {
          adblock::SubscriptionService::Snapshot snapshot;
          auto collection =
              std::make_unique<adblock::MockSubscriptionCollection>();
          // TODO(mpawlowski) will the collection be queried for classification?
          // If yes, add EXPECT_CALL(collection, ...) here.
          snapshot.push_back(std::move(collection));
          return snapshot;
        });
  }

  void OnDisconnect(adblock::AdblockURLLoaderFactory* factory) {
    EXPECT_EQ(factory, adblock_factory_.get());
    adblock_factory_.reset();
  }

  void ExpectCheckRewrite(HostState state = HostState::Alive) {
    EXPECT_CALL(
        resource_classifier_,
        CheckRewriteFilterMatch(testing::_, flow.url, testing::_, testing::_))
        .WillOnce(
            [&, state](
                auto, const GURL&, content::GlobalRenderFrameHostId,
                base::OnceCallback<void(const absl::optional<GURL>&)> cb) {
              adblock_factory_->ApplyState(state);
              std::move(cb).Run({});
            });
  }

  void ExpectNoCheckRewrite() {
    EXPECT_CALL(
        resource_classifier_,
        CheckRewriteFilterMatch(testing::_, testing::_, testing::_, testing::_))
        .Times(0);
  }

  void InitializeFlow() {
    ExpectCheckRewrite();
    EXPECT_CALL(resource_classifier_,
                CheckDocumentAllowlisted(testing::_, flow.url, testing::_))
        .Times(0);
    EXPECT_CALL(resource_classifier_,
                CheckRequestFilterMatch(testing::_, flow.url, testing::_,
                                        testing::_, testing::_))
        .WillOnce([&](auto, const GURL&, adblock::ContentType,
                      content::GlobalRenderFrameHostId,
                      adblock::CheckFilterMatchCallback cb) {
          std::move(cb).Run(flow.request_match);
        });
  }

  void InitializePopupFlow() {
    flow.destination = network::mojom::RequestDestination::kDocument;
    adblock::FrameOpenerInfo::CreateForWebContents(web_contents());
    auto* info = adblock::FrameOpenerInfo::FromWebContents(web_contents());
    info->SetOpener(main_rfh()->GetGlobalId());
    ExpectNoCheckRewrite();  // We never rewrite popups
    EXPECT_CALL(resource_classifier_,
                CheckDocumentAllowlisted(testing::_, flow.url, testing::_))
        .Times(0);
    EXPECT_CALL(
        resource_classifier_,
        CheckPopupFilterMatch(testing::_, flow.url, testing::_, testing::_))
        .WillOnce([&](auto, const GURL&, content::GlobalRenderFrameHostId,
                      adblock::CheckFilterMatchCallback cb) {
          std::move(cb).Run(flow.request_match);
        });
  }

  void ExpectRequestAllowed(HostState state = HostState::Alive) {
    EXPECT_CALL(resource_classifier_,
                CheckResponseFilterMatch(testing::_, flow.url, testing::_,
                                         testing::_, testing::_, testing::_))
        .WillOnce([&, state](auto, const GURL&, adblock::ContentType,
                             content::GlobalRenderFrameHostId, const auto&,
                             adblock::CheckFilterMatchCallback cb) {
          adblock_factory_->ApplyState(state);
          std::move(cb).Run(flow.response_match);
        });
  }

  void ExpectPopupAllowed(HostState state = HostState::Alive) {
    EXPECT_CALL(resource_classifier_,
                CheckResponseFilterMatch(testing::_, flow.url, testing::_,
                                         testing::_, testing::_, testing::_))
        .WillOnce([&, state](auto, const GURL&, adblock::ContentType,
                             content::GlobalRenderFrameHostId, const auto&,
                             adblock::CheckFilterMatchCallback cb) {
          adblock_factory_->ApplyState(state);
          std::move(cb).Run(flow.response_match);
        });
  }

  void ExpectRequestBlockedOrNotHappened() {
    // if request was not processed or blocked, response processing should not
    // take place.
    EXPECT_CALL(resource_classifier_,
                CheckResponseFilterMatch(testing::_, flow.url, testing::_,
                                         testing::_, testing::_, testing::_))
        .Times(0);
  }

  void ExpectElemhideDone() {
    EXPECT_CALL(element_hider_, IsElementTypeHideable(testing::_))
        .WillOnce(testing::Return(flow.element_hidable));
    EXPECT_CALL(element_hider_, HideBlockedElement(flow.url, testing::_))
        .Times(flow.element_hidable ? 1 : 0);
  }

  void ExpectElemhideSkipped() {
    EXPECT_CALL(element_hider_, IsElementTypeHideable(testing::_)).Times(0);
    EXPECT_CALL(element_hider_, HideBlockedElement(flow.url, testing::_))
        .Times(0);
  }

  void ExpectResponseAllowed(HostState state = HostState::Alive) {
    EXPECT_CALL(sitekey_storage_,
                ProcessResponseHeaders(flow.url, testing::_, kTestUserAgent))
        .Times(1);
    EXPECT_CALL(csp_injector_,
                InsertContentSecurityPolicyHeadersIfApplicable(
                    flow.url, testing::_, testing::_, testing::_))
        .WillOnce(
            [&, state](const GURL&, auto, auto,
                       adblock::InsertContentSecurityPolicyHeadersCallback cb) {
              adblock_factory_->ApplyState(state);
              std::move(cb).Run(nullptr);
            });
  }

  void ExpectResponseBlockedOrNotHappened() {
    // if response was not processed or blocked, headers processing should not
    // take place.
    EXPECT_CALL(sitekey_storage_,
                ProcessResponseHeaders(flow.url, testing::_, kTestUserAgent))
        .Times(0);
    EXPECT_CALL(csp_injector_,
                InsertContentSecurityPolicyHeadersIfApplicable(
                    flow.url, testing::_, testing::_, testing::_))
        .Times(0);
  }

  std::unique_ptr<network::SimpleURLLoader> loader_;
  std::unique_ptr<TestURLLoaderFactory> adblock_factory_;
  network::TestURLLoaderFactory test_factory_;
  mojo::Receiver<network::mojom::URLLoaderFactory> test_factory_receiver_;
  adblock::MockSubscriptionService subscription_service_;
  adblock::MockResourceClassificationRunner resource_classifier_;
  adblock::MockElementHider element_hider_;
  adblock::MockSitekeyStorage sitekey_storage_;
  adblock::MockAdblockContentSecurityPolicyInjector csp_injector_;
  std::vector<base::OnceClosure> deferred_tasks_;
  RequestFlow flow;
};

TEST_F(AdblockURLLoaderFactoryTest, HappyPath) {
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, BlockedWithRequestFilter) {
  flow.request_match = adblock::FilterMatchResult::kBlockRule;
  InitializeFlow();
  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, BlockedWithResponseFilter) {
  flow.response_match = adblock::FilterMatchResult::kBlockRule;
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, BlockedWithRequestFilterNonHideable) {
  flow.request_match = adblock::FilterMatchResult::kBlockRule;
  flow.element_hidable = false;
  InitializeFlow();
  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, BlockedWithResponseFilterNonHideable) {
  flow.response_match = adblock::FilterMatchResult::kBlockRule;
  flow.element_hidable = false;
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, DocumentNavigation) {
  flow.destination = network::mojom::RequestDestination::kDocument;
  ExpectNoCheckRewrite();  // We never rewrite document navigation.
  EXPECT_CALL(resource_classifier_,
              CheckDocumentAllowlisted(testing::_, flow.url, testing::_));
  EXPECT_CALL(resource_classifier_,
              CheckRequestFilterMatch(testing::_, flow.url, testing::_,
                                      testing::_, testing::_))
      .Times(0);

  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, PopupNavigation) {
  InitializePopupFlow();
  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, FrameDiesWhilePopupNavigation) {
  InitializePopupFlow();
  ExpectRequestAllowed(HostState::Dead);
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, FrameDiesWhileRewriteCheck) {
  ExpectCheckRewrite(HostState::Dead);
  EXPECT_CALL(resource_classifier_,
              CheckDocumentAllowlisted(testing::_, flow.url, testing::_))
      .Times(0);
  EXPECT_CALL(resource_classifier_,
              CheckRequestFilterMatch(testing::_, flow.url, testing::_,
                                      testing::_, testing::_))
      .Times(0);

  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, FrameDiesWhileRequestMatch) {
  InitializeFlow();
  ExpectRequestAllowed(HostState::Dead);
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, FrameDiesBeforeResponseMatch) {
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseAllowed(HostState::Dead);
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, SkipCspForNonDocument) {
  flow.destination = network::mojom::RequestDestination::kImage;
  InitializeFlow();
  ExpectRequestAllowed();
  EXPECT_CALL(sitekey_storage_,
              ProcessResponseHeaders(flow.url, testing::_, kTestUserAgent))
      .Times(0);
  EXPECT_CALL(csp_injector_, InsertContentSecurityPolicyHeadersIfApplicable(
                                 flow.url, testing::_, testing::_, testing::_))
      .Times(0);
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, Localhost) {
  flow.url = GURL("http://localhost:8080/test");
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_F(AdblockURLLoaderFactoryTest, NonHttp) {
  flow.url = GURL(
      "data:image/"
      "png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4/"
      "/8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==");
  flow.destination = network::mojom::RequestDestination::kImage;
  EXPECT_CALL(resource_classifier_,
              CheckRequestFilterMatch(testing::_, flow.url, testing::_,
                                      testing::_, testing::_))
      .Times(0);
  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectNoCheckRewrite();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}
