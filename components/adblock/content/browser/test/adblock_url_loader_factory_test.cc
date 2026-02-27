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
#include "components/adblock/content/browser/request_initiator.h"
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

enum class MakeRedirection {
  Yes,
  No,
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
  // If set this is the final url to which we redirect
  std::optional<GURL> redirect_url = std::nullopt;
  scoped_refptr<net::HttpResponseHeaders> headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("");
};

class AdblockURLLoaderFactoryTest
    : public content::RenderViewHostTestHarness,
      public testing::WithParamInterface<MakeRedirection> {
 public:
  AdblockURLLoaderFactoryTest() : test_factory_receiver_(&test_factory_) {}

  AdblockURLLoaderFactoryTest(const AdblockURLLoaderFactoryTest&) = delete;
  AdblockURLLoaderFactoryTest& operator=(const AdblockURLLoaderFactoryTest&) =
      delete;

  void MaybeMakeRedirection() {
    if (GetParam() == MakeRedirection::Yes) {
      flow.redirect_url = GURL{"https://example.com"};
    }
  }

  // Get urls for the flow. Note that in case we block initial request then
  // even if there is a redirect_url set it will not be hit.
  std::vector<GURL> GetUrlsForCurrentFlow() {
    std::vector<GURL> urls{flow.url};
    if (flow.redirect_url &&
        (flow.request_match != adblock::FilterMatchResult::kBlockRule)) {
      urls.push_back(flow.redirect_url.value());
    }
    return urls;
  }

  // If we don't block initial request and it redirects then we return
  // redirected url, otherwise initial url
  GURL GetUrlForResponseProcessing() {
    return (flow.redirect_url &&
            (flow.request_match != adblock::FilterMatchResult::kBlockRule))
               ? flow.redirect_url.value()
               : flow.url;
  }

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

    adblock_factory_ = std::make_unique<adblock::AdblockURLLoaderFactory>(
        adblock::AdblockURLLoaderFactoryConfig{
            &subscription_service_, &resource_classifier_, &element_hider_,
            &sitekey_storage_, &csp_injector_},
        adblock::RequestInitiator(web_contents()->GetPrimaryMainFrame()),
        std::move(factory_request),
        test_factory_receiver_.BindNewPipeAndPassRemote(), kTestUserAgent,
        base::BindOnce(&AdblockURLLoaderFactoryTest::OnDisconnect,
                       base::Unretained(this)));
    std::string body("Hello.");
    network::URLLoaderCompletionStatus status;
    status.decoded_body_length = body.size();
    auto head = network::mojom::URLResponseHead::New();
    head->headers = flow.headers;
    if (!flow.redirect_url) {
      test_factory_.AddResponse(flow.url, std::move(head), body, status);
    } else {
      network::TestURLLoaderFactory::Redirects redirects;
      net::RedirectInfo redirect_info;
      redirect_info.new_url = flow.redirect_url.value();
      redirects.push_back(
          {redirect_info, network::mojom::URLResponseHead::New()});
      test_factory_.AddResponse(flow.url, std::move(head), body, status,
                                std::move(redirects));
    }
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
                auto, const GURL&, const adblock::RequestInitiator&,
                base::OnceCallback<void(const absl::optional<GURL>&)> cb) {
              if (state == HostState::Dead) {
                // Simulate frame death that happens during the async execution.
                DeleteContents();
              }
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
    for (const auto& url : GetUrlsForCurrentFlow()) {
      EXPECT_CALL(resource_classifier_,
                  CheckDocumentAllowlisted(testing::_, url, testing::_))
          .Times(0);
      EXPECT_CALL(resource_classifier_,
                  CheckRequestFilterMatch(testing::_, url, testing::_,
                                          testing::_, testing::_))
          .WillOnce([&](auto, const GURL&, adblock::ContentType,
                        const adblock::RequestInitiator&,
                        adblock::CheckFilterMatchCallback cb) {
            std::move(cb).Run(flow.request_match);
          });
    }
  }

  void InitializePopupFlow() {
    flow.destination = network::mojom::RequestDestination::kDocument;
    adblock::FrameOpenerInfo::CreateForWebContents(web_contents());
    auto* info = adblock::FrameOpenerInfo::FromWebContents(web_contents());
    info->SetOpener(main_rfh()->GetGlobalId());
    ExpectNoCheckRewrite();  // We never rewrite popups
    for (const auto& url : GetUrlsForCurrentFlow()) {
      EXPECT_CALL(resource_classifier_,
                  CheckDocumentAllowlisted(testing::_, url, testing::_))
          .Times(0);
      EXPECT_CALL(
          resource_classifier_,
          CheckPopupFilterMatch(testing::_, url, testing::_, testing::_))
          .WillOnce([&](auto, const GURL&, content::RenderFrameHost&,
                        adblock::CheckFilterMatchCallback cb) {
            std::move(cb).Run(flow.request_match);
          });
    }
  }

  void ExpectRequestAllowed(HostState state = HostState::Alive) {
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(resource_classifier_,
                CheckResponseFilterMatch(testing::_, url, testing::_,
                                         testing::_, testing::_, testing::_))
        .WillOnce([&, state](auto, const GURL&, adblock::ContentType,
                             const adblock::RequestInitiator&, const auto&,
                             adblock::CheckFilterMatchCallback cb) {
          if (state == HostState::Dead) {
            // Simulate frame death that happens during the async execution.
            DeleteContents();
          }
          std::move(cb).Run(flow.response_match);
        });
  }

  void ExpectPopupAllowed(HostState state = HostState::Alive) {
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(resource_classifier_,
                CheckResponseFilterMatch(testing::_, url, testing::_,
                                         testing::_, testing::_, testing::_))
        .WillOnce([&, state](auto, const GURL&, adblock::ContentType,
                             const adblock::RequestInitiator&, const auto&,
                             adblock::CheckFilterMatchCallback cb) {
          if (state == HostState::Dead) {
            // Simulate frame death that happens during the async execution.
            DeleteContents();
          }
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
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(element_hider_, IsElementTypeHideable(testing::_))
        .WillOnce(testing::Return(flow.element_hidable));
    EXPECT_CALL(element_hider_, HideBlockedElement(url, testing::_))
        .Times(flow.element_hidable ? 1 : 0);
  }

  void ExpectElemhideSkipped() {
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(element_hider_, IsElementTypeHideable(testing::_)).Times(0);
    EXPECT_CALL(element_hider_, HideBlockedElement(url, testing::_)).Times(0);
  }

  void ExpectResponseAllowed(HostState state = HostState::Alive) {
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(sitekey_storage_,
                ProcessResponseHeaders(url, testing::_, kTestUserAgent))
        .Times(1);
    EXPECT_CALL(csp_injector_, InsertContentSecurityPolicyHeadersIfApplicable(
                                   url, testing::_, testing::_, testing::_))
        .WillOnce(
            [&, state](const GURL&, auto, auto,
                       adblock::InsertContentSecurityPolicyHeadersCallback cb) {
              if (state == HostState::Dead) {
                // Simulate frame death that happens during the async execution.
                DeleteContents();
              }
              std::move(cb).Run(nullptr);
            });
  }

  void ExpectResponseBlockedOrNotHappened() {
    // if response was not processed or blocked, headers processing should not
    // take place.
    GURL url = GetUrlForResponseProcessing();
    EXPECT_CALL(sitekey_storage_,
                ProcessResponseHeaders(url, testing::_, kTestUserAgent))
        .Times(0);
    EXPECT_CALL(csp_injector_, InsertContentSecurityPolicyHeadersIfApplicable(
                                   url, testing::_, testing::_, testing::_))
        .Times(0);
  }

  std::unique_ptr<network::SimpleURLLoader> loader_;
  std::unique_ptr<adblock::AdblockURLLoaderFactory> adblock_factory_;
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

TEST_P(AdblockURLLoaderFactoryTest, HappyPath) {
  MaybeMakeRedirection();
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, MissingResponseHeaders) {
  MaybeMakeRedirection();
  flow.headers = nullptr;
  InitializeFlow();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, BlockedWithRequestFilter) {
  MaybeMakeRedirection();
  flow.request_match = adblock::FilterMatchResult::kBlockRule;
  InitializeFlow();
  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, BlockedWithResponseFilter) {
  MaybeMakeRedirection();
  flow.response_match = adblock::FilterMatchResult::kBlockRule;
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, BlockedWithRequestFilterNonHideable) {
  MaybeMakeRedirection();
  flow.request_match = adblock::FilterMatchResult::kBlockRule;
  flow.element_hidable = false;
  InitializeFlow();
  ExpectRequestBlockedOrNotHappened();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, BlockedWithResponseFilterNonHideable) {
  MaybeMakeRedirection();
  flow.response_match = adblock::FilterMatchResult::kBlockRule;
  flow.element_hidable = false;
  InitializeFlow();
  ExpectRequestAllowed();
  ExpectResponseBlockedOrNotHappened();
  ExpectElemhideDone();
  StartRequest();
  EXPECT_EQ(net::ERR_BLOCKED_BY_ADMINISTRATOR, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, DocumentNavigation) {
  MaybeMakeRedirection();
  flow.destination = network::mojom::RequestDestination::kDocument;
  ExpectNoCheckRewrite();  // We never rewrite document navigation.
  for (const auto& url : GetUrlsForCurrentFlow()) {
    EXPECT_CALL(resource_classifier_,
                CheckRequestFilterMatch(testing::_, url, testing::_, testing::_,
                                        testing::_))
        .Times(0);
  }
  // We call CheckDocumentAllowlisted() when we receive a response so we do that
  // only for final url after all redirections
  EXPECT_CALL(resource_classifier_,
              CheckDocumentAllowlisted(
                  testing::_, GetUrlForResponseProcessing(), testing::_));

  ExpectRequestAllowed();
  ExpectResponseAllowed();
  ExpectElemhideSkipped();
  StartRequest();
  EXPECT_EQ(net::OK, loader_->NetError());
}

TEST_P(AdblockURLLoaderFactoryTest, PopupNavigation) {
  MaybeMakeRedirection();
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

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockURLLoaderFactoryTest,
                         testing::Values(MakeRedirection::Yes,
                                         MakeRedirection::No));
