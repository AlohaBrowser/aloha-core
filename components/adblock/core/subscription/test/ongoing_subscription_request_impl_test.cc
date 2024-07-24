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

#include "components/adblock/core/subscription/ongoing_subscription_request_impl.h"

#include <memory>
#include <string_view>

#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace adblock {

class AdblockOngoingSubscriptionRequestImplTest
    : public testing::TestWithParam<OngoingSubscriptionRequest::Method> {
 public:
  void SetUp() final {
    SetOnline();
    ongoing_request_ = std::make_unique<OngoingSubscriptionRequestImpl>(
        &kRetryBackoffPolicy, test_shared_url_loader_factory_);
  }

  std::string_view MethodAsString(OngoingSubscriptionRequest::Method method) {
    return method == OngoingSubscriptionRequest::Method::GET
               ? net::HttpRequestHeaders::kGetMethod
               : net::HttpRequestHeaders::kHeadMethod;
  }

  void VerifyRequestSent(OngoingSubscriptionRequest::Method method) {
    ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
    EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url, kUrl);
    EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.method,
              MethodAsString(method));
  }

  void VerifyRequestSent() { VerifyRequestSent(GetParam()); }

  void SetOffline() {
    network_change_notifier_->SetConnectionTypeAndNotifyObservers(
        net::NetworkChangeNotifier::CONNECTION_NONE);
  }

  void SetOnline() {
    network_change_notifier_->SetConnectionTypeAndNotifyObservers(
        net::NetworkChangeNotifier::CONNECTION_WIFI);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<net::test::MockNetworkChangeNotifier>
      network_change_notifier_ = net::test::MockNetworkChangeNotifier::Create();
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory>
      test_shared_url_loader_factory_{
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              &test_url_loader_factory_)};
  const GURL kUrl{"https://url.com/filter"};
  const net::BackoffEntry::Policy kRetryBackoffPolicy = {
      0,      // Number of initial errors to ignore.
      5000,   // Initial delay in ms.
      2.0,    // Factor by which the waiting time will be multiplied.
      0,      // Fuzzing percentage.
      10000,  // Maximum delay in ms.
      -1,     // Never discard the entry.
      false,  // Use initial delay.
  };
  std::unique_ptr<OngoingSubscriptionRequestImpl> ongoing_request_;
};

TEST_P(AdblockOngoingSubscriptionRequestImplTest,
       RequestDeferredUntilConnectionAvailable) {
  SetOffline();
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  // Download did not start yet.
  EXPECT_EQ(test_url_loader_factory_.NumPending(), 0);

  SetOnline();

  // Request started.
  VerifyRequestSent();
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest,
       RequestConnectionAvailableTriggersDownloadsOnlyAfterStart) {
  SetOffline();
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;

  // Download did not start yet.
  EXPECT_EQ(test_url_loader_factory_.NumPending(), 0);

  SetOnline();

  // Download did not start yet.
  EXPECT_EQ(test_url_loader_factory_.NumPending(), 0);

  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  // Request started.
  VerifyRequestSent();
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest,
       RequestCompletedSuccessfully) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  VerifyRequestSent();

  const std::string content = "downloaded content";

  auto header_response = network::CreateURLResponseHead(net::HTTP_OK);
  header_response->headers->AddHeader("Date", "Today");

  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .WillOnce([&](const GURL, base::FilePath downloaded_file,
                    scoped_refptr<net::HttpResponseHeaders> headers) {
        ASSERT_TRUE(headers);
        EXPECT_TRUE(headers->HasHeaderValue("Date", "Today"));
        // We expect a downloaded_file in GET mode, HEAD requests deliver
        // only headers.
        if (GetParam() == OngoingSubscriptionRequest::Method::GET) {
          std::string content_in_file;
          EXPECT_TRUE(
              base::ReadFileToString(downloaded_file, &content_in_file));
          EXPECT_EQ(content_in_file, content);
        } else {
          EXPECT_TRUE(downloaded_file.empty());
        }
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      kUrl, network::URLLoaderCompletionStatus(net::OK),
      std::move(header_response), content);
  task_environment_.RunUntilIdle();
  // No additional tasks are expected.
  EXPECT_EQ(task_environment_.GetPendingMainThreadTaskCount(), 0u);
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest,
       RetriesDeferredProgressively) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  VerifyRequestSent();

  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .WillRepeatedly([&](const GURL&, base::FilePath downloaded_file,
                          scoped_refptr<net::HttpResponseHeaders> headers) {
        // The response callback triggers a Retry(), simulating that the
        // received content was invalid.
        ongoing_request_->Retry();
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrl.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();
  // A retry attempt task has been posted.
  EXPECT_EQ(task_environment_.GetPendingMainThreadTaskCount(), 1u);
  // The delay matches the retry policy
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay().InMilliseconds(),
            kRetryBackoffPolicy.initial_delay_ms);

  // Fast-forward time until the retry task is executed.
  task_environment_.FastForwardBy(
      task_environment_.NextMainThreadPendingTaskDelay());

  // A retry request was sent.
  VerifyRequestSent();
  // The response comes back, triggers the response_callback which, again,
  // results in a retry.
  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrl.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();

  EXPECT_EQ(task_environment_.GetPendingMainThreadTaskCount(), 1u);
  // The delay is now multiplied, according to backoff policy.
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay().InMilliseconds(),
            kRetryBackoffPolicy.initial_delay_ms *
                kRetryBackoffPolicy.multiply_factor);
  // Fast-forward time until the retry task is executed.
  task_environment_.FastForwardBy(
      task_environment_.NextMainThreadPendingTaskDelay());
  // A retry request was sent.
  VerifyRequestSent();
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest, RequestCancelledDuringRetry) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  VerifyRequestSent();

  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .WillRepeatedly([&](const GURL&, base::FilePath downloaded_file,
                          scoped_refptr<net::HttpResponseHeaders> headers) {
        ongoing_request_->Retry();
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrl.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();
  // A retry attempt task has been posted.
  EXPECT_EQ(task_environment_.GetPendingMainThreadTaskCount(), 1u);
  // The delay matches the retry policy
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay().InMilliseconds(),
            kRetryBackoffPolicy.initial_delay_ms);

  // We now cancel the download by destroying ongoing_request_.
  ongoing_request_.reset();

  // Fast-forward time until the retry task was scheduled to execute.
  task_environment_.FastForwardBy(
      task_environment_.NextMainThreadPendingTaskDelay());

  // A retry request was not sent, as the request is cancelled.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest, RedirectCallStartsDownload) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  // Redirect counter is 0 by default
  EXPECT_EQ(ongoing_request_->GetNumberOfRedirects(), 0u);

  VerifyRequestSent();

  const GURL kRedirectUrl{"https://redirect_url.com"};
  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .WillOnce([&](const GURL&, base::FilePath downloaded_file,
                    scoped_refptr<net::HttpResponseHeaders> headers) {
        // The response callback triggers a Redirect()
        ongoing_request_->Redirect(kRedirectUrl);
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrl.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();

  // Redirect counter is incremented by 1
  EXPECT_EQ(ongoing_request_->GetNumberOfRedirects(), 1u);

  // A redirect request was sent with the redirect URL and same method
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url,
            kRedirectUrl);
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.method,
            MethodAsString(GetParam()));
}

TEST_P(AdblockOngoingSubscriptionRequestImplTest,
       RequestCancelledAfterStarting) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  VerifyRequestSent();

  // We now cancel the download by destroying ongoing_request_.
  ongoing_request_.reset();
  // The response callback will not be called after the request has been
  // cancelled...
  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .Times(0);
  // ... even when the response arrives.
  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrl.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockOngoingSubscriptionRequestImplTest,
       ResponseCallbackCalledWithTrimmedUrlGET) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  const GURL kUrlWithQPs{"https://url.com/filter?qp1=qp1_val&qp2=qp2_val"};
  ongoing_request_->Start(kUrlWithQPs, OngoingSubscriptionRequest::Method::GET,
                          response_callback.Get());
  EXPECT_CALL(response_callback, Run(kUrl, testing::_, testing::_)).Times(1);
  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrlWithQPs.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockOngoingSubscriptionRequestImplTest,
       ResponseCallbackCalledWithTrimmedUrlHEAD) {
  base::MockCallback<OngoingSubscriptionRequest::ResponseCallback>
      response_callback;
  const GURL kUrlWithQPs{"https://url.com/filter?qp1=qp1_val&qp2=qp2_val"};
  ongoing_request_->Start(kUrlWithQPs, OngoingSubscriptionRequest::Method::HEAD,
                          response_callback.Get());
  EXPECT_CALL(response_callback, Run(GURL(), testing::_, testing::_)).Times(1);
  test_url_loader_factory_.SimulateResponseForPendingRequest(kUrlWithQPs.spec(),
                                                             "content");
  task_environment_.RunUntilIdle();
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockOngoingSubscriptionRequestImplTest,
    testing::Values(OngoingSubscriptionRequest::Method::GET,
                    OngoingSubscriptionRequest::Method::HEAD));

}  // namespace adblock
