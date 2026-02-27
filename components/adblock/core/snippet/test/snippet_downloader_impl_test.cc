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

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_file.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/test/mock_task_scheduler.h"
#include "components/adblock/core/net/test/mock_adblock_resource_request.h"
#include "components/adblock/core/snippet/test/mock_snippet_storage.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "components/prefs/testing_pref_service.h"
#include "crypto/obsolete/md5.h"
#include "net/http/http_response_headers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockSnippetDownloaderImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    common::prefs::RegisterProfilePrefs(prefs_.registry());
  }

  std::unique_ptr<SnippetDownloaderImpl> CreateDownloader(
      std::optional<std::string> head_etag,
      std::optional<std::string> get_etag,
      std::optional<std::string> content) {
    auto updater = std::make_unique<MockTaskScheduler>();
    EXPECT_CALL(*updater, StartSchedule(testing::_))
        .WillOnce([](base::RepeatingClosure callback) { callback.Run(); });
    if (content) {
      EXPECT_TRUE(file_.Create());
      base::WriteFile(file_.path(), *content);
      downloaded_file_ = file_.path();
    }
    testing::InSequence sequence;
    if (head_etag) {
      head_headers_ = base::MakeRefCounted<net::HttpResponseHeaders>("");
      head_headers_->SetHeader("ETag", *head_etag);
      EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
        auto request = std::make_unique<MockAdblockResourceRequest>();
        EXPECT_CALL(
            *request,
            Start(url_, AdblockResourceRequest::Method::HEAD, testing::_,
                  AdblockResourceRequest::RetryPolicy::DoNotRetry, "",
                  testing::_))
            .WillOnce([&](GURL url, auto,
                          AdblockResourceRequest::ResponseCallback callback,
                          auto, const std::string,
                          const AdblockResourceRequest::HeadersMap) {
              std::move(callback).Run(url, base::FilePath{}, head_headers_);
            });
        return request;
      });
    }
    if (get_etag) {
      get_headers_ = base::MakeRefCounted<net::HttpResponseHeaders>("");
      get_headers_->SetHeader("ETag", *get_etag);
      EXPECT_CALL(request_maker_, Run()).WillOnce([&]() {
        auto request = std::make_unique<MockAdblockResourceRequest>();
        EXPECT_CALL(*request,
                    Start(url_, AdblockResourceRequest::Method::GET, testing::_,
                          AdblockResourceRequest::RetryPolicy::DoNotRetry, "",
                          testing::_))
            .WillOnce([&, f = file_.path()](
                          GURL url, auto,
                          AdblockResourceRequest::ResponseCallback callback,
                          auto, const std::string,
                          const AdblockResourceRequest::HeadersMap) {
              std::move(callback).Run(url, f, get_headers_);
            });
        return request;
      });
    }
    return std::make_unique<SnippetDownloaderImpl>(
        &prefs_, std::move(updater), request_maker_.Get(), url_, &storage_);
  }

  std::string CalculateEtag(const std::string& content) {
    auto hash =
        crypto::obsolete::Md5::HashForTesting(base::as_byte_span(content));
    return base::ToLowerASCII(base::HexEncode(hash));
  }

  std::string CalculateSignature(const std::string& content) {
    return SubscriptionValidatorImpl::ComputeSubscriptionHash(
        InMemoryFlatbufferData(content));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  MockSnippetStorage storage_;
  base::ScopedTempFile file_;
  GURL url_{"https//test.com/snippet"};
  std::optional<base::FilePath> downloaded_file_;
  base::MockCallback<SnippetDownloaderImpl::SnippetsRequestMaker>
      request_maker_;
  scoped_refptr<net::HttpResponseHeaders> get_headers_;
  scoped_refptr<net::HttpResponseHeaders> head_headers_;
};

TEST_F(AdblockSnippetDownloaderImplTest,
       FirstRunHeadThenGetThenStoreForNewEtag) {
  std::string old_library = "old conent";
  std::string new_library = "let snippets='';";
  std::string new_etag = "new_etag";
  auto updater = CreateDownloader(new_etag, new_etag, new_library);
  EXPECT_CALL(storage_, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(old_library));
  EXPECT_CALL(storage_, StoreSnippets(new_library, testing::_))
      .WillOnce([](const std::string&,
                   base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run("sig");
      });
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(new_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
  EXPECT_TRUE(downloaded_file_.has_value());
  EXPECT_FALSE(base::PathExists(*downloaded_file_));
}

TEST_F(AdblockSnippetDownloaderImplTest, FirstRunHeadThenNoGetIfNoNewEtag) {
  std::string old_library = "old conent";
  auto head_etag = CalculateEtag(old_library);
  auto updater = CreateDownloader(head_etag, std::nullopt, std::nullopt);
  EXPECT_CALL(storage_, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(old_library));
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(head_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
}

TEST_F(AdblockSnippetDownloaderImplTest, FirstRunHeadThenNoGetIfEmptyHeadEtag) {
  auto updater = CreateDownloader("", std::nullopt, std::nullopt);
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_EQ("", prefs_.GetString(common::prefs::kSnippetsVersion));
}

TEST_F(AdblockSnippetDownloaderImplTest,
       FirstRunHeadThenGetThenNoStoreIfEmptyGetEtag) {
  std::string library = "";
  std::string head_etag = "head_etag";
  auto updater = CreateDownloader(head_etag, "", std::nullopt);
  EXPECT_CALL(storage_, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(library));
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);
  updater->StartSchedule();
  EXPECT_EQ("", prefs_.GetString(common::prefs::kSnippetsVersion));
  task_environment_.RunUntilIdle();
}

TEST_F(AdblockSnippetDownloaderImplTest, GetAndStoreIfNewEtag) {
  std::string old_etag = "etag";
  std::string new_library = "let snippets='';";
  // Set kSnippetsVersion to skip HEAD
  prefs_.SetString(common::prefs::kSnippetsVersion, old_etag);
  std::string new_etag = "new_etag";
  auto updater = CreateDownloader(std::nullopt, new_etag, new_library);
  auto expected_signature = CalculateSignature(new_library);
  EXPECT_CALL(storage_, StoreSnippets(new_library, testing::_))
      .WillOnce([expected_signature](
                    const std::string&,
                    base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(expected_signature);
      });
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(downloaded_file_.has_value());
  EXPECT_FALSE(base::PathExists(*downloaded_file_));
  EXPECT_EQ(new_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
  EXPECT_EQ(expected_signature,
            prefs_.GetString(common::prefs::kSnippetsSignature));
}

// FIXME: Remove or enable depending on DPD-3415
TEST_F(AdblockSnippetDownloaderImplTest,
       DISABLED_GetAndNoStoreIfInvalidContent) {
  std::string old_etag = "etag";
  std::string new_library = "&&%)()*()%&&$$";
  // Set kSnippetsVersion to skip HEAD
  prefs_.SetString(common::prefs::kSnippetsVersion, old_etag);
  std::string new_etag = "new_etag";
  auto updater = CreateDownloader(std::nullopt, new_etag, new_library);
  EXPECT_CALL(storage_, StoreSnippets(new_library, testing::_)).Times(0);
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(downloaded_file_.has_value());
  EXPECT_FALSE(base::PathExists(*downloaded_file_));
  EXPECT_EQ(old_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
  EXPECT_TRUE(prefs_.GetString(common::prefs::kSnippetsSignature).empty());
}

TEST_F(AdblockSnippetDownloaderImplTest, GetAndNoStoreIfNoNewEtag) {
  std::string old_etag = "etag";
  // Set kSnippetsVersion to skip HEAD
  prefs_.SetString(common::prefs::kSnippetsVersion, old_etag);
  auto updater = CreateDownloader(std::nullopt, old_etag, std::nullopt);
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(old_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
  EXPECT_TRUE(prefs_.GetString(common::prefs::kSnippetsSignature).empty());
}

TEST_F(AdblockSnippetDownloaderImplTest, GetAndNoStoreIfEmptyEtag) {
  std::string old_etag = "etag";
  // Set kSnippetsVersion to skip HEAD
  prefs_.SetString(common::prefs::kSnippetsVersion, old_etag);
  auto updater = CreateDownloader(std::nullopt, "", std::nullopt);
  EXPECT_CALL(storage_, StoreSnippets(testing::_, testing::_)).Times(0);
  updater->StartSchedule();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(old_etag, prefs_.GetString(common::prefs::kSnippetsVersion));
  EXPECT_TRUE(prefs_.GetString(common::prefs::kSnippetsSignature).empty());
}

}  // namespace adblock
