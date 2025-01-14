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
#include "components/adblock/core/net/test/mock_adblock_resource_request.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/test/mock_conversion_executors.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/prefs/pref_service.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class FakeBuffer final : public FlatbufferData {
 public:
  const uint8_t* data() const final { return nullptr; }
  size_t size() const final { return 0u; }
  const base::span<const uint8_t> span() const final { return {}; }
};

class AdblockSubscriptionDownloaderImplTest : public testing::Test {
 public:
  AdblockSubscriptionDownloaderImplTest() {
    downloader_ = std::make_unique<SubscriptionDownloaderImpl>(
        request_maker_.Get(), &conversion_executor_, &persistent_metadata_);
  }

  void TestDateHeaderParsing(network::mojom::URLResponseHeadPtr header_response,
                             std::string_view expected_parsed_string) {
    base::MockCallback<SubscriptionDownloader::HeadRequestCallback>
        head_request_callback;

    // SubscriptionRequestMaker is asked to create a request,
    AdblockResourceRequest::ResponseCallback response_callback;
    EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
      // Expect that the request gets started, record the response callback.
      auto mock_ongoing_request =
          std::make_unique<MockAdblockResourceRequest>();
      EXPECT_CALL(*mock_ongoing_request,
                  Start(testing::_, AdblockResourceRequest::Method::HEAD,
                        testing::_, testing::_, testing::_))
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
                             AdblockResourceRequest::RetryPolicy::DoNotRetry,
                             callback.Get());
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       DownloadStarted_QueryParamsPresent) {
  EXPECT_CALL(persistent_metadata_, GetVersion(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return("555"));
  EXPECT_CALL(persistent_metadata_,
              GetDownloadSuccessCount(kSubscriptionUrlHttps))
      .WillRepeatedly(testing::Return(6));

  const std::string expected_extra_query_params =
      "lastVersion=555&disabled=false&downloadCount=4+&safe=true";
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(kSubscriptionUrlHttps, AdblockResourceRequest::Method::GET,
              testing::_, AdblockResourceRequest::RetryPolicy::DoNotRetry,
              expected_extra_query_params));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(kSubscriptionUrlHttps,
                             AdblockResourceRequest::RetryPolicy::DoNotRetry,
                             base::DoNothing());
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       ErrorCountIncreasedWhenDownloadResponseIsEmpty) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::GET,
                      testing::_, testing::_, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded,
      download_completed_callback.Get());

  // DownloadCompletedCallback will be called with nullptr.
  EXPECT_CALL(download_completed_callback, Run(testing::IsNull()));
  // AdblockResourceRequest calls ResponseCallback with empty path,
  // indicating no content. This will trigger IncrementDownloadErrorCount
  response_callback.Run(kSubscriptionUrlHttps, base::FilePath(), nullptr);
}

TEST_F(AdblockSubscriptionDownloaderImplTest,
       ConvertWhenDownloadResponseIsValid) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::GET,
                      testing::_, testing::_, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded,
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
  // AdblockResourceRequest calls ResponseCallback with a path to file with
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
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::GET,
                      testing::_, testing::_, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // The request gets redirected to the expected URL
    EXPECT_CALL(*mock_ongoing_request, Redirect(testing::_, testing::_));
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded,
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
  // AdblockResourceRequest calls ResponseCallback with a path to file with
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
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::GET,
                      testing::_, testing::_, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    // Redirect counter check gets called and returns kMaxNumberOfRedirects
    EXPECT_CALL(*mock_ongoing_request, GetNumberOfRedirects()).WillOnce([&]() {
      return SubscriptionDownloaderImpl::kMaxNumberOfRedirects;
    });
    return mock_ongoing_request;
  });

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded,
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
  // AdblockResourceRequest calls ResponseCallback with a path to file with
  // valid flatbuffer content:
  response_callback.Run(kSubscriptionUrlHttps, downloaded_flatbuffer_path,
                        nullptr);
  // The conversion will happen in a thread pool, we will need to run the entire
  // task environment to have all the callbacks execute.
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSubscriptionDownloaderImplTest, ConversionFailsTest) {
  base::MockCallback<SubscriptionDownloader::DownloadCompletedCallback>
      download_completed_callback;

  // SubscriptionRequestMaker is asked to create a request,
  AdblockResourceRequest::ResponseCallback response_callback;
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    testing::InSequence sequence;
    // Expect that the request gets started, record the response callback.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::GET,
                      testing::_, testing::_, testing::_))
        .WillOnce(testing::SaveArg<2>(&response_callback));
    return mock_ongoing_request;
  });

  EXPECT_CALL(persistent_metadata_,
              IncrementDownloadErrorCount(kSubscriptionUrlHttps));

  downloader_->StartDownload(
      kSubscriptionUrlHttps,
      AdblockResourceRequest::RetryPolicy::RetryUntilSucceeded,
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
  // AdblockResourceRequest calls ResponseCallback with a path to file with
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

  const std::string expected_extra_query_params =
      "lastVersion=222&disabled=true&downloadCount=3&safe=true";
  // SubscriptionRequestMaker is asked to create a request,
  EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
    // Expect that the request gets started, record the requested url.
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(
        *mock_ongoing_request,
        Start(kSubscriptionUrlHttps, AdblockResourceRequest::Method::HEAD,
              testing::_, AdblockResourceRequest::RetryPolicy::DoNotRetry,
              expected_extra_query_params));
    return mock_ongoing_request;
  });

  downloader_->DoHeadRequest(kSubscriptionUrlHttps, base::DoNothing());
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
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::HEAD,
                      testing::_, testing::_, testing::_));
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
    auto mock_ongoing_request = std::make_unique<MockAdblockResourceRequest>();
    EXPECT_CALL(*mock_ongoing_request,
                Start(testing::_, AdblockResourceRequest::Method::HEAD,
                      testing::_, testing::_, testing::_));
    return mock_ongoing_request;
  });
  downloader_->DoHeadRequest(kSubscriptionUrlHttps, base::DoNothing());
}

}  // namespace adblock
