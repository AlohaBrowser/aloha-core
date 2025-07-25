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

#include "components/adblock/core/net/adblock_resource_request_impl.h"

#include <memory>
#include <string_view>

#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/app_info.h"
#include "components/adblock/core/net/test/mock_adblock_request_throttle.h"
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

class AdblockResourceRequestImplTest
    : public testing::TestWithParam<AdblockResourceRequest::Method> {
 public:
  void SetUp() final {
    SetOnline();
    ongoing_request_ = std::make_unique<AdblockResourceRequestImpl>(
        &kRetryBackoffPolicy, test_shared_url_loader_factory_,
        &mock_request_throttle_);
  }

  std::string_view MethodAsString(AdblockResourceRequest::Method method) {
    return method == AdblockResourceRequest::Method::GET
               ? net::HttpRequestHeaders::kGetMethod
               : net::HttpRequestHeaders::kHeadMethod;
  }

  void VerifyRequestSent(AdblockResourceRequest::Method method) {
    ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
    EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url,
              UrlWithExpectedParams(kUrl));
    EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.method,
              MethodAsString(method));
  }

  void VerifyRequestSent() { VerifyRequestSent(GetParam()); }

  void SetOffline() { mock_request_throttle_.requests_allowed_ = false; }

  void SetOnline() {
    mock_request_throttle_.OverrideDelayImmediatelyForTesting();
  }

  const GURL UrlWithExpectedParams(const GURL& url,
                                   const std::string& extra_query_params = "") {
    std::string query = base::StrCat(
        {"addonName=", "eyeo-chromium-sdk", "&addonVersion=", "2.0.0",
         "&application=",
         base::EscapeQueryParamValue(AppInfo::Get().name, true),
         "&applicationVersion=",
         base::EscapeQueryParamValue(AppInfo::Get().version, true),
         "&platform=",
         base::EscapeQueryParamValue(AppInfo::Get().client_os, true),
         "&platformVersion=", "1.0"});

    if (!extra_query_params.empty()) {
      query += "&";
      query += extra_query_params;
    }

    GURL::Replacements replacements;
    replacements.SetQueryStr(query);
    return url.ReplaceComponents(replacements);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory test_url_loader_factory_;
  MockAdblockRequestThrottle mock_request_throttle_;
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
  std::unique_ptr<AdblockResourceRequestImpl> ongoing_request_;
};

TEST_P(AdblockResourceRequestImplTest,
       RequestDeferredUntilConnectionAvailable) {
  SetOffline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  // Download did not start yet.
  EXPECT_EQ(test_url_loader_factory_.NumPending(), 0);

  SetOnline();

  // Request started.
  VerifyRequestSent();
}

TEST_P(AdblockResourceRequestImplTest,
       RequestConnectionAvailableTriggersDownloadsOnlyAfterStart) {
  SetOffline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
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

TEST_P(AdblockResourceRequestImplTest, RequestCompletedSuccessfully) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
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
        if (GetParam() == AdblockResourceRequest::Method::GET) {
          std::string content_in_file;
          EXPECT_TRUE(
              base::ReadFileToString(downloaded_file, &content_in_file));
          EXPECT_EQ(content_in_file, content);
        } else {
          EXPECT_TRUE(downloaded_file.empty());
        }
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl), network::URLLoaderCompletionStatus(net::OK),
      std::move(header_response), content);
  task_environment_.RunUntilIdle();
  // No additional tasks are expected.
  EXPECT_EQ(task_environment_.GetPendingMainThreadTaskCount(), 0u);
}

TEST_P(AdblockResourceRequestImplTest, RetriesDeferredProgressively) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(
      kUrl, GetParam(), response_callback.Get(),
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded);

  VerifyRequestSent();

  // Response callback not called since the download failed.
  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .Times(0);

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED), nullptr, "");

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
  // The response comes back, with a net::Err which, again,
  // results in a retry.
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED), nullptr, "");

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

TEST_P(AdblockResourceRequestImplTest, RequestCancelledDuringRetry) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(
      kUrl, GetParam(), response_callback.Get(),
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded);

  VerifyRequestSent();

  // Response callback not called since the download failed.
  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .Times(0);

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED), nullptr, "");

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

TEST_P(AdblockResourceRequestImplTest, RedirectCallStartsDownload) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
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

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl).spec(), "content");
  task_environment_.RunUntilIdle();

  // Redirect counter is incremented by 1
  EXPECT_EQ(ongoing_request_->GetNumberOfRedirects(), 1u);

  // A redirect request was sent with the redirect URL and same method
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url,
            UrlWithExpectedParams(kRedirectUrl));
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.method,
            MethodAsString(GetParam()));
}

TEST_P(AdblockResourceRequestImplTest, RequestCancelledAfterStarting) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
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
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl).spec(), "content");
  task_environment_.RunUntilIdle();
}

TEST_P(AdblockResourceRequestImplTest, ExtraQueryParamsAddedForRequest) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;

  const std::string extra_query_param = "extra_key=extra_value";
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get(),
                          AdblockResourceRequest::RetryPolicy::DoNotRetry,
                          extra_query_param);

  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url,
            UrlWithExpectedParams(kUrl, extra_query_param));

  task_environment_.RunUntilIdle();
}

TEST_P(AdblockResourceRequestImplTest,
       ExtraQueryParamsAddedForRedirectedRequest) {
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, GetParam(), response_callback.Get());

  VerifyRequestSent();

  const GURL kRedirectUrl{"https://redirect_url.com"};
  const std::string extra_query_param = "extra_key=extra_value";
  EXPECT_CALL(response_callback, Run(testing::_, testing::_, testing::_))
      .WillOnce([&](const GURL&, base::FilePath downloaded_file,
                    scoped_refptr<net::HttpResponseHeaders> headers) {
        // The response callback triggers a Redirect()
        ongoing_request_->Redirect(kRedirectUrl, extra_query_param);
      });

  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl).spec(), "content");
  task_environment_.RunUntilIdle();

  // A redirect request was sent with query parameters
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  EXPECT_EQ(test_url_loader_factory_.GetPendingRequest(0)->request.url,
            UrlWithExpectedParams(kRedirectUrl, extra_query_param));
}

TEST_F(AdblockResourceRequestImplTest,
       ResponseCallbackCalledWithTrimmedUrlGET) {
  SetOnline();
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, AdblockResourceRequest::Method::GET,
                          response_callback.Get());

  VerifyRequestSent(AdblockResourceRequest::Method::GET);

  EXPECT_CALL(response_callback, Run(kUrl, testing::_, testing::_)).Times(1);
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl).spec(), "content");
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockResourceRequestImplTest,
       ResponseCallbackCalledWithTrimmedUrlHEAD) {
  base::MockCallback<AdblockResourceRequest::ResponseCallback>
      response_callback;
  ongoing_request_->Start(kUrl, AdblockResourceRequest::Method::HEAD,
                          response_callback.Get());

  VerifyRequestSent(AdblockResourceRequest::Method::HEAD);

  EXPECT_CALL(response_callback, Run(GURL(), testing::_, testing::_)).Times(1);
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      UrlWithExpectedParams(kUrl).spec(), "content");
  task_environment_.RunUntilIdle();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockResourceRequestImplTest,
                         testing::Values(AdblockResourceRequest::Method::GET,
                                         AdblockResourceRequest::Method::HEAD));

}  // namespace adblock
