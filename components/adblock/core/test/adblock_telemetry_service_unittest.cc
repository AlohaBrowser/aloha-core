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

#include "components/adblock/core/adblock_telemetry_service.h"

#include <memory>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/configuration/test/mock_filtering_configuration.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

namespace adblock {

namespace {

class MockTopicProvider
    : public NiceMock<AdblockTelemetryService::TopicProvider> {
 public:
  MOCK_METHOD(GURL, GetEndpointURL, (), (const, override));
  MOCK_METHOD(std::string, GetAuthToken, (), (const, override));
  MOCK_METHOD(void, GetPayload, (PayloadCallback), (const, override));
  MOCK_METHOD(base::Time, GetTimeOfNextRequest, (), (const, override));
  MOCK_METHOD(void, ParseResponse, (std::unique_ptr<std::string>), (override));
  MOCK_METHOD(void, FetchDebugInfo, (DebugInfoCallback), (const, override));
};

const auto kInitialDelay = base::Seconds(30);
const auto kCheckInterval = base::Minutes(5);
}  // namespace

class AdblockTelemetryServiceTest : public testing::Test {
 public:
  void SetUp() override {
    static const std::string adblock_name = kAdblockFilteringConfigurationName;
    EXPECT_CALL(filtering_configuration_, GetName())
        .WillRepeatedly(testing::ReturnRef(adblock_name));
    EXPECT_CALL(subscription_service_,
                GetFilteringConfiguration(kAdblockFilteringConfigurationName))
        .WillRepeatedly(testing::Return(&filtering_configuration_));
    telemetry_service_ = std::make_unique<AdblockTelemetryService>(
        &subscription_service_, test_shared_url_loader_factory_, kInitialDelay,
        kCheckInterval);
  }

  MockTopicProvider* RegisterFooMock() {
    auto* mock_provider = RegisterNewMockTopicProvider();
    ON_CALL(*mock_provider, GetEndpointURL()).WillByDefault(Return(kFooUrl));
    ON_CALL(*mock_provider, GetAuthToken()).WillByDefault(Return("foo_token"));
    ON_CALL(*mock_provider, GetPayload(testing::_))
        .WillByDefault([](MockTopicProvider::PayloadCallback callback) {
          std::move(callback).Run("foo_data");
        });
    ON_CALL(*mock_provider, GetTimeOfNextRequest())
        .WillByDefault(Return(base::Time::Now() + kFooDelay));
    return mock_provider;
  }

  void ExpectFooMadeRequest(int pending_request_idx) {
    const auto& resource_request =
        test_url_loader_factory_.GetPendingRequest(pending_request_idx)
            ->request;
    EXPECT_EQ(resource_request.url, kFooUrl);
    AssertRequestContainsRequiredData(resource_request, "foo_data",
                                      "foo_token");
  }

  MockTopicProvider* RegisterBarMock() {
    auto* mock_provider = RegisterNewMockTopicProvider();
    ON_CALL(*mock_provider, GetEndpointURL()).WillByDefault(Return(kBarUrl));
    ON_CALL(*mock_provider, GetAuthToken()).WillByDefault(Return("bar_token"));
    ON_CALL(*mock_provider, GetPayload(testing::_))
        .WillByDefault([](MockTopicProvider::PayloadCallback callback) {
          std::move(callback).Run("bar_data");
        });
    ON_CALL(*mock_provider, GetTimeOfNextRequest())
        .WillByDefault(Return(base::Time::Now() + kBarDelay));
    return mock_provider;
  }

  void ExpectBarMadeRequest(int pending_request_idx) {
    const auto& resource_request =
        test_url_loader_factory_.GetPendingRequest(pending_request_idx)
            ->request;
    EXPECT_EQ(resource_request.url, kBarUrl);
    AssertRequestContainsRequiredData(resource_request, "bar_data",
                                      "bar_token");
  }

 protected:
  MockTopicProvider* RegisterNewMockTopicProvider() {
    auto provider = std::make_unique<MockTopicProvider>();
    auto* provider_bare_ptr = provider.get();
    telemetry_service_->AddTopicProvider(std::move(provider));
    return provider_bare_ptr;
  }

  void AssertRequestContainsRequiredData(
      const network::ResourceRequest& resource_request,
      const std::string& expected_upload_data,
      const std::string& expected_auth_token) {
    // Cookies are not sent nor stored:
    EXPECT_FALSE(resource_request.SavesCookies());
    EXPECT_FALSE(resource_request.SendsCookies());

    // Authorization token is being sent:
    std::string auth_header;
    ASSERT_TRUE(resource_request.headers.GetHeader(
        net::HttpRequestHeaders::kAuthorization, &auth_header));
    EXPECT_EQ(auth_header, "Bearer " + expected_auth_token);

    // "Accept: application/json" is sent.
    std::string accept_header;
    ASSERT_TRUE(resource_request.headers.GetHeader(
        net::HttpRequestHeaders::kAccept, &accept_header));
    EXPECT_EQ(accept_header, "application/json");

    // Cache is being bypassed:
    EXPECT_EQ(resource_request.load_flags,
              net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE);

    // Payload is being sent:
    std::string upload_data;
    for (const auto& elem : *(resource_request.request_body->elements())) {
      auto piece = elem.As<network::DataElementBytes>().AsStringPiece();
      upload_data.append(piece.data(), piece.size());
    }
    EXPECT_EQ(upload_data, expected_upload_data);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  MockFilteringConfiguration filtering_configuration_;
  MockSubscriptionService subscription_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory>
      test_shared_url_loader_factory_{
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              &test_url_loader_factory_)};
  constexpr static base::TimeDelta kFooDelay{base::Minutes(4)};
  constexpr static base::TimeDelta kBarDelay{base::Minutes(6)};
  const GURL kFooUrl{"https://telemetry.com/topic/eyeo_foo/version/3"};
  const GURL kBarUrl{"https://telemetry.com/topic/eyeo_bar/version/2"};

  std::unique_ptr<AdblockTelemetryService> telemetry_service_;
};

TEST_F(AdblockTelemetryServiceTest, RequestNotMadeBeforeInitialDelay) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  auto* mock = RegisterFooMock();
  EXPECT_CALL(*mock, GetTimeOfNextRequest())
      .WillOnce(Return(base::Time::Now()));

  telemetry_service_->Start();

  // Despite the Foo topic provider declaring a next request time of Now(),
  // we wait until kInitialDelay has passed.
  task_environment_.RunUntilIdle();
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // Only after initial delay has passed, there's a network request made.
  task_environment_.FastForwardBy(kInitialDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest,
       ScheduleStartsImmediatelyWhenAdblockEnabled) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  RegisterFooMock();

  telemetry_service_->Start();

  // There are no TopicProviders that are due already after the initial delay.
  task_environment_.FastForwardBy(kInitialDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // The topic should have scheduled a request according to its
  // GetTimeOfNextRequest(). It will be checked after kCheckInterval.
  task_environment_.FastForwardBy(kCheckInterval);

  // A request was sent to a URL built according to topic provider's data.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, ScheduleStartupDelayedWhenAdblockDisabled) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(false));

  RegisterFooMock();

  telemetry_service_->Start();

  // No requests initially.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // A lot of time passes.
  task_environment_.FastForwardBy(kCheckInterval * 5);
  // There was no network request made, because IsAdblockEnabled() is false.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // IsAdblockEnabled() becomes true:
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));
  for (auto& o : filtering_configuration_.observers_) {
    o.OnEnabledStateChanged(&filtering_configuration_);
  }

  // Schedule is started, first request made after kInitialDelay.
  task_environment_.FastForwardBy(kInitialDelay);

  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, ScheduleAbortedWhenAdblockDisabled) {
  // Schedule starts normally, without delay:
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));
  RegisterFooMock();
  telemetry_service_->Start();

  // User disables Adblocking.
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(false));
  for (auto& o : filtering_configuration_.observers_) {
    o.OnEnabledStateChanged(&filtering_configuration_);
  }

  // A lot of time passes with no requests, schedule is stopped.
  task_environment_.FastForwardBy(kCheckInterval * 5);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);
}

TEST_F(AdblockTelemetryServiceTest,
       ScheduleAbortedWhenAdblockRemovedThenStartsWhenInstalled) {
  // Schedule starts normally, without delay:
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));
  RegisterFooMock();
  telemetry_service_->Start();

  // "adblock" configuration is removed.
  subscription_service_.observer_->OnFilteringConfigurationUninstalled(
      kAdblockFilteringConfigurationName);

  // A lot of time passes with no requests, schedule is stopped.
  task_environment_.FastForwardBy(kCheckInterval * 5);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // "custom" configuration is added again => still no pings.
  const std::string custom_name = "custom";
  MockFilteringConfiguration custom_filtering_configuration;
  EXPECT_CALL(custom_filtering_configuration, IsEnabled())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(custom_filtering_configuration, GetName())
      .WillRepeatedly(testing::ReturnRef(custom_name));
  subscription_service_.observer_->OnFilteringConfigurationInstalled(
      &custom_filtering_configuration);
  task_environment_.FastForwardBy(kCheckInterval * 5);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // "adblock" configuration is added again.
  subscription_service_.observer_->OnFilteringConfigurationInstalled(
      &filtering_configuration_);

  // Schedule is started, first request made after kInitialDelay.
  task_environment_.FastForwardBy(kInitialDelay);

  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, MultipleProvidersMakeRequests) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  RegisterFooMock();
  RegisterBarMock();

  telemetry_service_->Start();

  // No requests initially.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // Time for Foo topic provider to make a request.
  task_environment_.FastForwardBy(kInitialDelay);
  task_environment_.FastForwardBy(kCheckInterval);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  // Time for Bar topic provider to make a request.
  task_environment_.FastForwardBy(kCheckInterval);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 2);
  ExpectBarMadeRequest(1);
}

TEST_F(AdblockTelemetryServiceTest, SuccessfulResponseReceived) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  auto* mock = RegisterFooMock();

  telemetry_service_->Start();

  task_environment_.FastForwardBy(kInitialDelay);
  task_environment_.FastForwardBy(kCheckInterval);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  const std::string response = "response_content";

  // Response will trigger TopicProvider's ParseResponse().
  EXPECT_CALL(*mock, ParseResponse(testing::Pointee(response)));
  test_url_loader_factory_.SimulateResponseForPendingRequest(kFooUrl.spec(),
                                                             response);
}

TEST_F(AdblockTelemetryServiceTest, Non200ResponseStillParsed) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  auto* mock = RegisterFooMock();

  telemetry_service_->Start();

  task_environment_.FastForwardBy(kInitialDelay);
  task_environment_.FastForwardBy(kCheckInterval);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  const std::string response = "error_content";

  // Even with a non-200 response code, TopicProvider is still shown the body
  // of the response. The response may contain an error description which the
  // TopicProvider may want to parse.
  EXPECT_CALL(*mock, ParseResponse(testing::Pointee(response)));
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      kFooUrl.spec(), response, net::HTTP_FORBIDDEN);
}

TEST_F(AdblockTelemetryServiceTest, RequestAbortedWhenAdblockDisabled) {
  // Start schedule normally:
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));
  auto* mock = RegisterFooMock();
  telemetry_service_->Start();
  task_environment_.FastForwardBy(kInitialDelay);
  task_environment_.FastForwardBy(kCheckInterval);
  // Request is made:
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  // Adblocking is disabled before response arrives:
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(false));
  for (auto& o : filtering_configuration_.observers_) {
    o.OnEnabledStateChanged(&filtering_configuration_);
  }

  // Now, TopicProvider is not triggered even when response arrives.
  EXPECT_CALL(*mock, ParseResponse(testing::_)).Times(0);

  test_url_loader_factory_.SimulateResponseForPendingRequest(kFooUrl.spec(),
                                                             "response");
}

TEST_F(AdblockTelemetryServiceTest, NegativeTimeToNextRequest) {
  EXPECT_CALL(filtering_configuration_, IsEnabled())
      .WillRepeatedly(Return(true));

  auto* mock = RegisterFooMock();
  // TopicProvider returns a negative time to next request, as if the request
  // was supposed to happen in the past. This is a normal scenario, ex. when
  // the browser was shut down for longer than the duration of request interval.
  EXPECT_CALL(*mock, GetTimeOfNextRequest())
      .WillOnce(testing::Return(base::Time::Now() - base::Seconds(30)));

  telemetry_service_->Start();
  task_environment_.FastForwardBy(kInitialDelay);
  // Request is made:
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, CollectDebugInfoFromProviders) {
  auto* foo_mock = RegisterFooMock();
  auto* bar_mock = RegisterBarMock();
  base::MockCallback<AdblockTelemetryService::TopicProvidersDebugInfoCallback>
      service_callback;

  // foo_mock provides the debug info synchronously.
  EXPECT_CALL(*foo_mock, FetchDebugInfo(testing::_))
      .WillOnce([](MockTopicProvider::DebugInfoCallback callback) {
        std::move(callback).Run("foo_debug_info");
      });

  // bar_mock provides the debug info asynchronously, after foo_mock.
  AdblockTelemetryService::TopicProvider::DebugInfoCallback bar_callback;
  EXPECT_CALL(*bar_mock, FetchDebugInfo(testing::_))
      .WillOnce([&bar_callback](MockTopicProvider::DebugInfoCallback callback) {
        bar_callback = std::move(callback);
      });

  // The service callback is called only after both providers have provided
  // their debug info.
  // Not yet:
  EXPECT_CALL(service_callback, Run(testing::_)).Times(0);
  telemetry_service_->GetTopicProvidersDebugInfo(service_callback.Get());

  // Now bar_mock provides its debug info.
  EXPECT_CALL(service_callback,
              Run(testing::ElementsAre("foo_debug_info", "bar_debug_info")));
  std::move(bar_callback).Run("bar_debug_info");
}

}  // namespace adblock
