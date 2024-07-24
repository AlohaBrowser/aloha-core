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

#include "components/adblock/core/subscription/subscription_downloader_impl.h"

#include <memory>
#include <string_view>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_split.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/test/mock_conversion_executors.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/prefs/pref_service.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::NiceMock;

namespace adblock {

class MockOngoingRequest : public NiceMock<OngoingSubscriptionRequest> {
 public:
  MOCK_METHOD(void,
              Start,
              (GURL url, Method method, ResponseCallback response_callback),
              (override));
  MOCK_METHOD(void, Retry, (), (override));
  MOCK_METHOD(void, Redirect, (GURL redirect_url), (override));
  MOCK_METHOD(size_t, GetNumberOfRedirects, (), (const, override));
};

class FakeBuffer final : public FlatbufferData {
 public:
  const uint8_t* data() const final { return nullptr; }
  size_t size() const final { return 0u; }
};

class AdblockSubscriptionDownloaderImplTest : public testing::Test {
 public:
  AdblockSubscriptionDownloaderImplTest() {
    app_info_.name = "test_app";
    app_info_.version = "123";
    app_info_.client_os = "Linux";
    downloader_ = std::make_unique<SubscriptionDownloaderImpl>(
        app_info_, request_maker_.Get(), &conversion_executor_,
        &persistent_metadata_);
  }

  void TestDateHeaderParsing(network::mojom::URLResponseHeadPtr header_response,
                             std::string_view expected_parsed_string) {
    base::MockCallback<SubscriptionDownloader::HeadRequestCallback>
        head_request_callback;

    // SubscriptionRequestMaker is asked to create a request,
    OngoingSubscriptionRequest::ResponseCallback response_callback;
    EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
      // Expect that the request gets started, record the response callback.
      auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
      EXPECT_CALL(*mock_ongoing_request,
                  Start(testing::_, OngoingSubscriptionRequest::Method::HEAD,
                        testing::_))
          .WillOnce(testing::SaveArg<2>(&response_callback));
      return mock_ongoing_request;
    });

    downloader_->DoHeadRequest(kSubscriptionUrlHttps,
                               head_request_callback.Get());

    // The HeadRequestCallback is called with parsed date, or empty string if
    // parsing failed.
    EXPECT_CALL(head_request_callback,
                Run(testing::StrEq(expected_parsed_string)));
    response_callback.Run(GURL(), base::FilePath(), header_response->headers);
  }

  base::test::TaskEnvironment task_environment_;
  utils::AppInfo app_info_;
  base::MockCallback<SubscriptionDownloaderImpl::SubscriptionRequestMaker>
      request_maker_;
  MockConversionExecutors conversion_executor_;
  MockSubscriptionPersistentMetadata persistent_metadata_;
  std::unique_ptr<SubscriptionDownloaderImpl> downloader_;

  const GURL kSubscriptionUrlHttps{"https://subscription.com/filterlist.txt"};
};

TEST_F(AdblockSubscriptionDownloaderImplTest,
       NoDownloadFromNotAllowedUrlScheme) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      callback;
  // Callback is immediately run with Null since the download cannot start.
  EXPECT_CALL(callback, Run(testing::IsNull()));
  // No requests are made.
  EXPECT_CALL(request_maker_, Run()).Times(0);

  const GURL kSubscriptionUrlHttp{"http://subscription.com/filterlist.txt"};
  downloader_->StartDownload(kSubscriptionUrlHttp,
                             SubscriptionDownloader::RetryPolicy::DoNotRetry,
                             callback.Get());
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       DownloadStarted_QueryParamsPresent) {
  EXPECT_CALL(persistent_metadata_, GetVersion(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return("555"));
  EXPECT_CALL(persistent_metadata_,
              GetDownloadSuccessCount(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return(6));

  GURL requested_url;
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<0>(&requested_url));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(kSubscriptionUrlHttps,
                             SubscriptionDownloader::RetryPolicy::DoNotRetry,
                             base::DoNothing());

  const auto query_params =
      base::SplitStringPiece(requested_url.query_piece(), "&",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  EXPECT_THAT(query_params,
              testing::UnorderedElementsAre(
                  "addonName=eyeo-chromium-sdk", "addonVersion=2.0.0",
                  "application=test_app", "applicationVersion=123",
                  "platform=Linux", "platformVersion=1.0", "lastVersion=555",
                  "disabled=false", "downloadCount=4+"));

  // The request's base url is |subscription_url|.
  GURL::Replacements strip_query;
  strip_query.ClearQuery();
  EXPECT_EQ(requested_url.ReplaceComponents(strip_query),
            kSubscriptionUrlHttps);
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       RetryWhenDownloadResponseIsEmpty) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // There will be a retry attempt due to empty response.
    EXPECT_CALL(*mock_ongoing_request, Retry());
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  // DownloadCompletedCallback will not be called because the download gets
  // retried.
  EXPECT_CALL(download_completed_callback, Run(testing::_)).Times(0);
  // OngoingSubscriptionRequest calls ResponseCallback with empty path,
  // indicating no content. This will trigger a retry.
  response_callback.Run(kSubscriptionUrlHttps, base::FilePath(), nullptr);
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       DoNotRetryWhenDownloadResponseIsEmptyIfPolicyForbids) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // There will be NO retry attempt due to empty response because of
    // RetryPolicy.
    EXPECT_CALL(*mock_ongoing_request, Retry()).Times(0);
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(kSubscriptionUrlHttps,
                             SubscriptionDownloader::RetryPolicy::DoNotRetry,
                             download_completed_callback.Get());

  // DownloadCompletedCallback will be called with nullptr;
  EXPECT_CALL(download_completed_callback, Run(testing::IsNull()));
  // OngoingSubscriptionRequest calls ResponseCallback with empty path,
  // indicating no content.
  response_callback.Run(kSubscriptionUrlHttps, base::FilePath(), nullptr);
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       ConvertWhenDownloadResponseIsValid) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  const base::FilePath downloaded_flatbuffer_path(FILE_PATH_LITERAL("file.fb"));
  // DownloadCompletedCallback will be called with a correctly converted
  // FlatbufferData.
  EXPECT_CALL(download_completed_callback, Run(testing::NotNull()));
  EXPECT_CALL(conversion_executor_,
              ConvertFilterListFile(kSubscriptionUrlHttps,
                                    downloaded_flatbuffer_path, testing::_))
      .WillOnce(testing::WithArgs<2>(
          testing::Invoke([](base::OnceCallback<void(ConversionResult)> cb) {
            std::move(cb).Run(std::make_unique<FakeBuffer>());
          })));
  // OngoingSubscriptionRequest calls ResponseCallback with a path to file with
  // valid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       RedirectWhenConverterResultIsRedirect) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;
  const GURL redirect_url =
      GURL("https://redirect_subscription.com/filterlist.txt");

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // The request gets redirected to the expected URL
    EXPECT_CALL(*mock_ongoing_request, Redirect(testing::_));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  const base::FilePath downloaded_flatbuffer_path(FILE_PATH_LITERAL("file.fb"));
  // DownloadCompletedCallback will not be called
  EXPECT_CALL(download_completed_callback, Run(testing::_)).Times(0);
  EXPECT_CALL(conversion_executor_,
              ConvertFilterListFile(kSubscriptionUrlHttps,
                                    downloaded_flatbuffer_path, testing::_))
      .WillOnce(testing::WithArgs<2>(testing::Invoke(
          [&redirect_url](base::OnceCallback<void(ConversionResult)> cb) {
            std::move(cb).Run(redirect_url);
          })));
  // OngoingSubscriptionRequest calls ResponseCallback with a path to file with
  // valid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       AbortWhenExceedingMaxNumberOfRedirects) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;
  const GURL redirect_url =
      GURL("https://redirect_subscription.com/filterlist.txt");

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // Redirect counter check gets called and returns kMaxNumberOfRedirects
    EXPECT_CALL(*mock_ongoing_request, GetNumberOfRedirects()).WillOnce([&]() {
      return SubscriptionDownloaderImpl::kMaxNumberOfRedirects;
    });
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  const base::FilePath downloaded_flatbuffer_path(FILE_PATH_LITERAL("file.fb"));
  // DownloadCompletedCallback will be called with null due to exceeding max
  // number of redirects
  EXPECT_CALL(download_completed_callback, Run(testing::IsNull())).Times(1);
  EXPECT_CALL(conversion_executor_,
              ConvertFilterListFile(kSubscriptionUrlHttps,
                                    downloaded_flatbuffer_path, testing::_))
      .WillOnce(testing::WithArgs<2>(testing::Invoke(
          [&redirect_url](base::OnceCallback<void(ConversionResult)> cb) {
            std::move(cb).Run(redirect_url);
          })));
  // OngoingSubscriptionRequest calls ResponseCallback with a path to file with
  // valid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest, NoRetryWhenConversionFailed) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // There should not be a retry since download was successful
    EXPECT_CALL(*mock_ongoing_request, Retry()).Times(0);
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  const base::FilePath downloaded_flatbuffer_path(FILE_PATH_LITERAL("file.fb"));

  // DownloadCompletedCallback gets called with nullptr, due to conversion
  // error.
  EXPECT_CALL(download_completed_callback, Run(testing::IsNull()));
  EXPECT_CALL(conversion_executor_,
              ConvertFilterListFile(kSubscriptionUrlHttps,
                                    downloaded_flatbuffer_path, testing::_))
      .WillOnce(testing::WithArgs<2>(
          testing::Invoke([](base::OnceCallback<void(ConversionResult)> cb) {
            std::move(cb).Run(ConversionError("Error"));
          })));
  // OngoingSubscriptionRequest calls ResponseCallback with a path to file with
  // invalid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       DoNotRetryWhenConversionFailedIfPolicyForbids) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  OngoingSubscriptionRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::GET, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // There will be NO retry attempt due to conversion failure because of
    // RetryPolicy.
    EXPECT_CALL(*mock_ongoing_request, Retry()).Times(0);
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(kSubscriptionUrlHttps,
                             SubscriptionDownloader::RetryPolicy::DoNotRetry,
                             download_completed_callback.Get());

  const base::FilePath downloaded_flatbuffer_path(FILE_PATH_LITERAL("file.fb"));
  // DownloadCompletedCallback will be called with nullptr.
  EXPECT_CALL(download_completed_callback, Run(testing::IsNull()));
  EXPECT_CALL(conversion_executor_,
              ConvertFilterListFile(kSubscriptionUrlHttps,
                                    downloaded_flatbuffer_path, testing::_))
      .WillOnce(testing::WithArgs<2>(
          testing::Invoke([](base::OnceCallback<void(ConversionResult)> cb) {
            std::move(cb).Run(ConversionError("Error"));
          })));
  // OngoingSubscriptionRequest calls ResponseCallback with a path to file with
  // invalid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       HeadRequestReturnsConverterHeaderDate) {
  auto header_response = network::CreateURLResponseHead(net::HTTP_OK);
  header_response->headers->AddHeader("Date", "Mon, 27 Sep 2021 13:53:01 GMT");
  TestDateHeaderParsing(std::move(header_response), "202109271353");
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       HeadRequestReturnsConverterHeaderDateLowerCaseDate) {
  auto header_response = network::CreateURLResponseHead(net::HTTP_OK);
  header_response->headers->AddHeader("date", "Mon, 27 Sep 2021 13:53:01 GMT");
  TestDateHeaderParsing(std::move(header_response), "202109271353");
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       HeadRequestReturnsConverterHeaderDateMalformed) {
  auto header_response = network::CreateURLResponseHead(net::HTTP_OK);
  header_response->headers->AddHeader("Date", "Invalid format");
  TestDateHeaderParsing(std::move(header_response), "");
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       HeadRequestReturnsConverterHeaderDateMissing) {
  auto header_response = network::CreateURLResponseHead(net::HTTP_OK);
  TestDateHeaderParsing(std::move(header_response), "");
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       HeadRequestSetsSubscriptionAsDisabled) {
  EXPECT_CALL(persistent_metadata_, GetVersion(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return("222"));
  EXPECT_CALL(persistent_metadata_,
              GetDownloadSuccessCount(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return(3));

  GURL requested_url;
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(testing::_, OngoingSubscriptionRequest::Method::HEAD, testing::_))
        .WillOnce(testing::SaveArg<0>(&requested_url));
    return mock_ongoing_request;
  });

  downloader_->DoHeadRequest(kSubscriptionUrlHttps, base::DoNothing());

  // The request's URL contains expected query parameters.
  const auto query_params =
      base::SplitStringPiece(requested_url.query_piece(), "&",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  EXPECT_THAT(query_params,
              testing::UnorderedElementsAre(
                  "addonName=eyeo-chromium-sdk", "addonVersion=2.0.0",
                  "application=test_app", "applicationVersion=123",
                  "platform=Linux", "platformVersion=1.0", "lastVersion=222",
                  "disabled=true", "downloadCount=3"));

  // The request's base url is |subscription_url|.
  GURL::Replacements strip_query;
  strip_query.ClearQuery();
  EXPECT_EQ(requested_url.ReplaceComponents(strip_query),
            kSubscriptionUrlHttps);
}

TEST_F(AdblockSubscriptionDownloaderImplTest, TwoConcurrentDoHeadRequests) {
  EXPECT_CALL(persistent_metadata_, GetVersion(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return("222"));
  EXPECT_CALL(persistent_metadata_,
              GetDownloadSuccessCount(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return(3));

  testing::StrictMock<
      base::MockCallback<SubscriptionDownloader::HeadRequestCallback>>
      head_request_callback;
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, OngoingSubscriptionRequest::Method::HEAD,
                      testing::_));
    return mock_ongoing_request;
  });
  downloader_->DoHeadRequest(kSubscriptionUrlHttps,
                             head_request_callback.Get());

  // The HeadRequestCallback is called with empty version string before
  // concurrent ping overrides ongoing one.
  EXPECT_CALL(head_request_callback, Run(testing::StrEq("")));
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockOngoingRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, OngoingSubscriptionRequest::Method::HEAD,
                      testing::_));
    return mock_ongoing_request;
  });
  downloader_->DoHeadRequest(kSubscriptionUrlHttps, base::DoNothing());
}

}  // namespace adblock
