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

#include "chrome/browser/adblock/adblock_chrome_content_browser_client.h"

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/gmock_move_support.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "gmock/gmock.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Ref;
using testing::Return;

namespace adblock {

class AdblockChromeContentBrowserClientUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  TestingProfile::TestingFactories GetTestingFactories() const override {
    return {TestingProfile::TestingFactory{
                SubscriptionServiceFactory::GetInstance(),
                base::BindRepeating([](content::BrowserContext* bc)
                                        -> std::unique_ptr<KeyedService> {
                  return std::make_unique<MockSubscriptionService>();
                })},
            TestingProfile::TestingFactory{
                ResourceClassificationRunnerFactory::GetInstance(),
                base::BindRepeating([](content::BrowserContext* bc)
                                        -> std::unique_ptr<KeyedService> {
                  return std::make_unique<MockResourceClassificationRunner>();
                })}};
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    subscription_service_ = static_cast<MockSubscriptionService*>(
        SubscriptionServiceFactory::GetForBrowserContext(profile()));
    resource_classification_runner_ =
        static_cast<MockResourceClassificationRunner*>(
            ResourceClassificationRunnerFactory::GetForBrowserContext(
                profile()));
  }

  void TearDown() override {
    subscription_service_ = nullptr;
    resource_classification_runner_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
  }

  raw_ptr<MockSubscriptionService> subscription_service_;
  raw_ptr<MockResourceClassificationRunner> resource_classification_runner_;
};

TEST_F(AdblockChromeContentBrowserClientUnitTest,
       WillInterceptWebSocketWhenFilteringEnabled) {
  AdblockChromeContentBrowserClient content_client;
  subscription_service_->WillRequireFiltering(true);
  EXPECT_TRUE(content_client.WillInterceptWebSocket(main_rfh()));
}

TEST_F(AdblockChromeContentBrowserClientUnitTest,
       WillNotInterceptWebSocketWhenFilteringDisabled) {
  AdblockChromeContentBrowserClient content_client;
  subscription_service_->WillRequireFiltering(false);
  EXPECT_FALSE(content_client.WillInterceptWebSocket(main_rfh()));
}

TEST_F(AdblockChromeContentBrowserClientUnitTest,
       RenderFrameHostDiesBeforeClassificationFinished) {
  const auto kSocketUrl = GURL("wss://domain.com/test");
  subscription_service_->WillRequireFiltering(true);
  EXPECT_CALL(*subscription_service_, GetCurrentSnapshot()).WillOnce([]() {
    SubscriptionService::Snapshot snapshot;
    snapshot.push_back(std::make_unique<MockSubscriptionCollection>());
    return snapshot;
  });
  CheckFilterMatchCallback classification_callback;
  EXPECT_CALL(*resource_classification_runner_,
              CheckRequestFilterMatch(_, kSocketUrl, ContentType::Websocket,
                                      RequestInitiator(main_rfh()), _))
      .WillOnce(MoveArg<4>(&classification_callback));

  AdblockChromeContentBrowserClient content_client;
  base::MockCallback<content::ContentBrowserClient::WebSocketFactory>
      web_socket_factory;
  // The web_socket_factory callback will never be called because the
  // associated RenderFrameHost will be dead.
  EXPECT_CALL(web_socket_factory, Run(_, _, _, _, _)).Times(0);

  const net::SiteForCookies site_for_cookies;
  content_client.CreateWebSocket(main_rfh(), web_socket_factory.Get(),
                                 kSocketUrl, site_for_cookies, absl::nullopt,
                                 {});
  // Tab is closed.
  DeleteContents();

  // Classification finishes now. It will not trigger a call to
  // |web_socket_factory| because the RFH is dead.
  std::move(classification_callback).Run(FilterMatchResult::kBlockRule);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockChromeContentBrowserClientUnitTest, WebSocketAllowed) {
  subscription_service_->WillRequireFiltering(true);
  const auto kSocketUrl = GURL("wss://domain.com/test");
  EXPECT_CALL(*subscription_service_, GetCurrentSnapshot()).WillOnce([]() {
    SubscriptionService::Snapshot snapshot;
    snapshot.push_back(std::make_unique<MockSubscriptionCollection>());
    return snapshot;
  });
  CheckFilterMatchCallback classification_callback;
  EXPECT_CALL(*resource_classification_runner_,
              CheckRequestFilterMatch(_, kSocketUrl, ContentType::Websocket,
                                      RequestInitiator(main_rfh()), _))
      .WillOnce(MoveArg<4>(&classification_callback));

  AdblockChromeContentBrowserClient content_client;
  base::MockCallback<content::ContentBrowserClient::WebSocketFactory>
      web_socket_factory;
  // The web_socket_factory callback will be called to let the web socket
  // continue connecting.
  EXPECT_CALL(web_socket_factory, Run(kSocketUrl, _, _, _, _));

  const net::SiteForCookies site_for_cookies;
  content_client.CreateWebSocket(main_rfh(), web_socket_factory.Get(),
                                 kSocketUrl, site_for_cookies, absl::nullopt,
                                 {});

  // Classification finishes now. It will trigger a call to |web_socket_factory|
  std::move(classification_callback).Run(FilterMatchResult::kAllowRule);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockChromeContentBrowserClientUnitTest, WebSocketBlocked) {
  subscription_service_->WillRequireFiltering(true);
  const auto kSocketUrl = GURL("wss://domain.com/test");
  EXPECT_CALL(*subscription_service_, GetCurrentSnapshot()).WillOnce([]() {
    SubscriptionService::Snapshot snapshot;
    snapshot.push_back(std::make_unique<MockSubscriptionCollection>());
    return snapshot;
  });
  CheckFilterMatchCallback classification_callback;
  EXPECT_CALL(*resource_classification_runner_,
              CheckRequestFilterMatch(_, kSocketUrl, ContentType::Websocket,
                                      RequestInitiator(main_rfh()), _))
      .WillOnce(MoveArg<4>(&classification_callback));

  AdblockChromeContentBrowserClient content_client;
  base::MockCallback<content::ContentBrowserClient::WebSocketFactory>
      web_socket_factory;
  // The web_socket_factory callback will not be called as to disallow
  // connection.
  EXPECT_CALL(web_socket_factory, Run(kSocketUrl, _, _, _, _)).Times(0);

  const net::SiteForCookies site_for_cookies;
  content_client.CreateWebSocket(main_rfh(), web_socket_factory.Get(),
                                 kSocketUrl, site_for_cookies, absl::nullopt,
                                 {});

  // Classification finishes now.
  std::move(classification_callback).Run(FilterMatchResult::kBlockRule);

  task_environment()->RunUntilIdle();
}

}  // namespace adblock
