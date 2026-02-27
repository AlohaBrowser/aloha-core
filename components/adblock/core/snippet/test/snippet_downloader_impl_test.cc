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

#include "components/adblock/core/snippet/snippet_downloader_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_file.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/test/mock_task_scheduler.h"
#include "components/adblock/core/net/test/mock_adblock_resource_request.h"
#include "components/adblock/core/snippet/test/mock_snippet_storage.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_version.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

namespace {
constexpr char kIfNoneMatch[] = "If-None-Match";
// Entity-tags as a server sends them. The quotes are part of the syntax, not
// decoration, which is what StoredVersionKeepsServerEtagQuoting pins down.
constexpr char kEtagV1[] = "\"v1\"";
constexpr char kEtagV2[] = "\"v2\"";
}  // namespace

class AdblockSnippetDownloaderImplTest : public ::testing::Test {
 public:
  // One canned HTTP response. |content|, when set, is written to a temp file
  // and served as the downloaded body; when unset the request yields no file,
  // which is what AdblockResourceRequest reports for any non-2xx response.
  struct Response {
    std::string status;
    std::optional<std::string> etag;
    std::optional<std::string> content;
  };

  // One observed outgoing request.
  struct Request {
    GURL url;
    AdblockResourceRequest::Method method;
    AdblockResourceRequest::RetryPolicy retry_policy;
    AdblockResourceRequest::HeadersMap headers;
  };

  void SetUp() override {
    common::prefs::RegisterProfilePrefs(prefs_.registry());
  }

  // Serves |responses| in order to successive requests, recording each request
  // in |requests_|. The scheduled update check is captured rather than run, so
  // a test can fire it once per simulated interval via RunCheck().
  std::unique_ptr<SnippetDownloaderImpl> CreateDownloader(
      std::vector<Response> responses) {
    responses_ = std::move(responses);
    auto scheduler = std::make_unique<MockTaskScheduler>();
    EXPECT_CALL(*scheduler, StartSchedule(testing::_))
        .WillOnce([this](base::RepeatingClosure callback) {
          update_check_ = std::move(callback);
        });
    EXPECT_CALL(request_maker_, Run()).WillRepeatedly([this]() {
      auto request = std::make_unique<MockAdblockResourceRequest>();
      EXPECT_CALL(*request, Start)
          .WillOnce([this](GURL url, AdblockResourceRequest::Method method,
                           AdblockResourceRequest::ResponseCallback callback,
                           AdblockResourceRequest::RetryPolicy retry_policy,
                           const std::string extra_query_params,
                           const AdblockResourceRequest::HeadersMap headers) {
            const size_t index = requests_.size();
            requests_.push_back({url, method, retry_policy, headers});
            ASSERT_LT(index, responses_.size())
                << "unexpected request number " << index + 1;
            std::move(callback).Run(url, ServeFile(responses_[index]),
                                    MakeHeaders(responses_[index]));
          });
      return request;
    });
    return std::make_unique<SnippetDownloaderImpl>(
        &prefs_, std::move(scheduler), request_maker_.Get(), url_, &storage_);
  }

  // Fires one scheduled update check and lets it run to completion.
  void RunCheck() {
    CHECK(update_check_) << "StartSchedule() was not called";
    update_check_.Run();
    task_environment_.RunUntilIdle();
  }

  std::string CalculateSignature(const std::string& content) {
    return SubscriptionValidatorImpl::ComputeSubscriptionHash(
        InMemoryFlatbufferData(content));
  }

  std::string StoredVersion() {
    return prefs_.GetString(common::prefs::kSnippetsVersion);
  }

  std::string StoredSignature() {
    return prefs_.GetString(common::prefs::kSnippetsSignature);
  }

 protected:
  base::FilePath ServeFile(const Response& response) {
    if (!response.content) {
      return base::FilePath();
    }
    auto file = std::make_unique<base::ScopedTempFile>();
    CHECK(file->Create());
    CHECK(base::WriteFile(file->path(), *response.content));
    auto path = file->path();
    served_files_.push_back(std::move(file));
    return path;
  }

  scoped_refptr<net::HttpResponseHeaders> MakeHeaders(
      const Response& response) {
    net::HttpResponseHeaders::Builder builder(net::HttpVersion(1, 1),
                                              response.status);
    if (response.etag) {
      builder.AddHeader("ETag", *response.etag);
    }
    return builder.Build();
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  MockSnippetStorage storage_;
  GURL url_{"https://test.com/snippet"};
  base::MockCallback<SnippetDownloaderImpl::SnippetsRequestMaker>
      request_maker_;
  std::vector<Response> responses_;
  std::vector<Request> requests_;
  std::vector<std::unique_ptr<base::ScopedTempFile>> served_files_;
  base::RepeatingClosure update_check_;
};

// A profile with no stored version has nothing to revalidate against, so it
// fetches unconditionally. It must not send a HEAD first.
TEST_F(AdblockSnippetDownloaderImplTest, FirstRunSendsOneUnconditionalGet) {
  const std::string library = "let snippets='';";
  auto downloader = CreateDownloader({{"200 OK", kEtagV1, library}});
  EXPECT_CALL(storage_, StoreSnippets(library, testing::_))
      .WillOnce([this](const std::string& content,
                       base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(CalculateSignature(content));
      });

  downloader->StartSchedule();
  RunCheck();

  ASSERT_EQ(1u, requests_.size());
  EXPECT_EQ(url_, requests_[0].url);
  EXPECT_EQ(AdblockResourceRequest::Method::GET, requests_[0].method);
  EXPECT_FALSE(requests_[0].headers.contains(kIfNoneMatch));
  // A 304 yields no file, which RetryUntilSucceeded would read as a failure.
  EXPECT_EQ(AdblockResourceRequest::RetryPolicy::DoNotRetry,
            requests_[0].retry_policy);
}

// The ETag is opaque: its quotes are part of the entity-tag syntax and a server
// will not match an unquoted value, so it is stored exactly as received.
TEST_F(AdblockSnippetDownloaderImplTest, StoredVersionKeepsServerEtagQuoting) {
  const std::string library = "let snippets='';";
  auto downloader = CreateDownloader({{"200 OK", kEtagV1, library}});
  EXPECT_CALL(storage_, StoreSnippets(library, testing::_))
      .WillOnce([this](const std::string& content,
                       base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(CalculateSignature(content));
      });

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ(CalculateSignature(library), StoredSignature());
}

// The whole point of storing the ETag: the next check revalidates with it,
// byte for byte, and the server answers 304 instead of resending the library.
TEST_F(AdblockSnippetDownloaderImplTest,
       SecondCheckRevalidatesWithStoredEtagVerbatim) {
  const std::string library = "let snippets='';";
  auto downloader = CreateDownloader(
      {{"200 OK", kEtagV1, library}, {"304 Not Modified", kEtagV1, {}}});
  EXPECT_CALL(storage_, StoreSnippets(library, testing::_))
      .WillOnce([this](const std::string& content,
                       base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(CalculateSignature(content));
      });

  downloader->StartSchedule();
  RunCheck();
  RunCheck();

  ASSERT_EQ(2u, requests_.size());
  EXPECT_FALSE(requests_[0].headers.contains(kIfNoneMatch));
  ASSERT_TRUE(requests_[1].headers.contains(kIfNoneMatch));
  EXPECT_EQ(kEtagV1, requests_[1].headers.at(kIfNoneMatch));
  EXPECT_EQ(AdblockResourceRequest::RetryPolicy::DoNotRetry,
            requests_[1].retry_policy);
}

TEST_F(AdblockSnippetDownloaderImplTest,
       NotModifiedLeavesStoredLibraryUntouched) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  prefs_.SetString(common::prefs::kSnippetsSignature, "signature");
  auto downloader = CreateDownloader({{"304 Not Modified", kEtagV1, {}}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ("signature", StoredSignature());
}

// A 304 carries no body, so there is nothing to store even if its ETag is not
// the one we revalidated with. Storing the absent body would erase the library.
TEST_F(AdblockSnippetDownloaderImplTest,
       NotModifiedWithUnexpectedEtagDoesNotEraseLibrary) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  prefs_.SetString(common::prefs::kSnippetsSignature, "signature");
  auto downloader = CreateDownloader({{"304 Not Modified", kEtagV2, {}}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ("signature", StoredSignature());
}

// An error page can carry its own ETag. It is not a snippet library.
TEST_F(AdblockSnippetDownloaderImplTest, ServerErrorDoesNotEraseLibrary) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  prefs_.SetString(common::prefs::kSnippetsSignature, "signature");
  auto downloader =
      CreateDownloader({{"500 Internal Server Error", "\"error\"", {}}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ("signature", StoredSignature());
}

// A 206 body is a fragment of the library, and nothing downstream parses the
// content, so only the status code can reject it. Adopting it would also
// persist the full representation's ETag, after which every check revalidates
// to a 304 and the truncated library never gets replaced.
TEST_F(AdblockSnippetDownloaderImplTest, PartialContentIsNotStored) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  prefs_.SetString(common::prefs::kSnippetsSignature, "signature");
  auto downloader =
      CreateDownloader({{"206 Partial Content", kEtagV2, "let snip"}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ("signature", StoredSignature());
}

// A download that cannot be read back yields empty content. Storing it would
// leave the browser with no snippets until the next restart.
TEST_F(AdblockSnippetDownloaderImplTest, EmptyDownloadedContentIsNotStored) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  prefs_.SetString(common::prefs::kSnippetsSignature, "signature");
  auto downloader = CreateDownloader({{"200 OK", kEtagV2, std::string()}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_EQ("signature", StoredSignature());
}

TEST_F(AdblockSnippetDownloaderImplTest, NewLibraryIsStoredAndTempFileRemoved) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  const std::string library = "let snippets='new';";
  auto downloader = CreateDownloader({{"200 OK", kEtagV2, library}});
  EXPECT_CALL(storage_, StoreSnippets(library, testing::_))
      .WillOnce([this](const std::string& content,
                       base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(CalculateSignature(content));
      });

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV2, StoredVersion());
  EXPECT_EQ(CalculateSignature(library), StoredSignature());
  ASSERT_EQ(1u, served_files_.size());
  EXPECT_FALSE(base::PathExists(served_files_[0]->path()));
}

// Without an ETag there is nothing to revalidate with next time, so the
// response cannot be adopted as the current version.
TEST_F(AdblockSnippetDownloaderImplTest, ResponseWithoutEtagIsIgnored) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  auto downloader =
      CreateDownloader({{"200 OK", std::nullopt, "let snippets='new';"}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_TRUE(StoredSignature().empty());
}

TEST_F(AdblockSnippetDownloaderImplTest, ResponseWithEmptyEtagIsIgnored) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  auto downloader = CreateDownloader({{"200 OK", "", "let snippets='new';"}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_TRUE(StoredSignature().empty());
}

// FIXME: Remove or enable depending on DPD-3415
TEST_F(AdblockSnippetDownloaderImplTest, DISABLED_InvalidContentIsNotStored) {
  prefs_.SetString(common::prefs::kSnippetsVersion, kEtagV1);
  auto downloader = CreateDownloader({{"200 OK", kEtagV2, "&&%)()*()%&&$$"}});
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);

  downloader->StartSchedule();
  RunCheck();

  EXPECT_EQ(kEtagV1, StoredVersion());
  EXPECT_TRUE(StoredSignature().empty());
}

}  // namespace adblock
