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

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/ranges/algorithm.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/test/bind.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "components/adblock/content/browser/factories/subscription_persistent_metadata_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/version_info/version_info.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockSubscriptionServiceBrowserTest
    : public AdblockBrowserTestBase,
      public SubscriptionService::SubscriptionObserver {
 public:
  AdblockSubscriptionServiceBrowserTest() {
    SubscriptionServiceFactory::SetUpdateCheckIntervalForTesting(
        base::Seconds(1));
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
  }

  bool RequestHeadersContainAcceptLanguage(
      const net::test_server::HttpRequest& request) {
    const auto accept_language_it =
        request.headers.find(net::HttpRequestHeaders::kAcceptLanguage);
    return accept_language_it != request.headers.end() &&
           !accept_language_it->second.empty();
  }

  bool RequestHeadersContainAcceptEncodingBrotli(
      const net::test_server::HttpRequest& request) {
    const auto accept_encoding_it =
        request.headers.find(net::HttpRequestHeaders::kAcceptEncoding);
    if (accept_encoding_it == request.headers.end()) {
      return false;
    }
    const auto split_encodings =
        base::SplitString(accept_encoding_it->second, ",",
                          base::WhitespaceHandling::TRIM_WHITESPACE,
                          base::SplitResult::SPLIT_WANT_NONEMPTY);
    return base::ranges::find(split_encodings, "br") != split_encodings.end();
  }

  std::unique_ptr<net::test_server::HttpResponse>
  HandleSubscriptionUpdateRequestWithUrlCheck(
      std::string expected_url_part,
      const net::test_server::HttpRequest& request) {
    static const char kSubscriptionHeader[] =
        "[Adblock Plus 2.0]\n"
        "! Checksum: X5A8vtJDBW2a9EgS9glqbg\n"
        "! Version: 202202061935\n"
        "! Last modified: 06 Feb 2022 19:35 UTC\n"
        "! Expires: 1 days (update frequency)\n\n";
    if (base::StartsWith(request.relative_url, kSubscription,
                         base::CompareCase::SENSITIVE) &&
        !request_already_handled_) {
      request_already_handled_ = true;
      EXPECT_TRUE(RequestHeadersContainAcceptLanguage(request));
      EXPECT_TRUE(RequestHeadersContainAcceptEncodingBrotli(request));
      std::string os;
      base::ReplaceChars(version_info::GetOSType(), base::kWhitespaceASCII, "",
                         &os);
      EXPECT_TRUE(request.relative_url.find(expected_url_part) !=
                  std::string::npos);
      EXPECT_TRUE(request.relative_url.find("addonName=eyeo-chromium-sdk") !=
                  std::string::npos);
      EXPECT_TRUE(request.relative_url.find("addonVersion=2.0.0") !=
                  std::string::npos);
      EXPECT_TRUE(request.relative_url.find("platformVersion=1.0") !=
                  std::string::npos);
      EXPECT_TRUE(request.relative_url.find("platform=" + os) !=
                  std::string::npos);
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kSubscriptionHeader);
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

  void ExpectFilterListRequestMadeWithLastVersion(std::string last_version) {
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&AdblockSubscriptionServiceBrowserTest::
                                HandleSubscriptionUpdateRequestWithUrlCheck,
                            base::Unretained(this), last_version));
    ASSERT_TRUE(https_server_->Start(kPort));
  }

  // adblock::SubscriptionService::SubscriptionObserver
  void OnSubscriptionInstalled(const GURL& url) override {
    if (base::StartsWith(url.spec(),
                         https_server_->GetURL(kSubscription).spec())) {
      // In order to ensure next run requests an update, mark the subscription
      // as almost expired.
      SubscriptionPersistentMetadataFactory::GetForBrowserContext(
          browser_context())
          ->SetExpirationInterval(url, base::Milliseconds(1));
      NotifyTestFinished();
    }
  }

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  bool request_already_handled_ = false;
  static const char kSubscription[];
  // Port is hardcoded so the server url is the same across tests
  static const int kPort = 65432;
};

const char AdblockSubscriptionServiceBrowserTest::kSubscription[] =
    "/subscription.txt";

IN_PROC_BROWSER_TEST_F(AdblockSubscriptionServiceBrowserTest, PRE_LastVersion) {
  ExpectFilterListRequestMadeWithLastVersion("&lastVersion=0&");
  // Downloading a filter list and setting its expiry time to almost zero, so
  // the next run will have to update it ASAP.

  auto* subscription_service =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context());
  subscription_service->AddObserver(this);
  // Using a custom subscription URL here because before test sets
  // up the server then SubscriptionService already started fetching default
  // subscriptions.
  subscription_service
      ->GetFilteringConfiguration(kAdblockFilteringConfigurationName)
      ->AddFilterList(https_server_->GetURL(kSubscription));
  // Wait until subscription is downloaded and stored.
  RunUntilTestFinished();
}

IN_PROC_BROWSER_TEST_F(AdblockSubscriptionServiceBrowserTest, LastVersion) {
  ExpectFilterListRequestMadeWithLastVersion("&lastVersion=202202061935&");
  auto* subscription_service =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context());
  subscription_service->AddObserver(this);
  // Wait for subscription update to trigger a network request.
  RunUntilTestFinished();
}

IN_PROC_BROWSER_TEST_F(AdblockSubscriptionServiceBrowserTest,
                       FilterFileDeletedAfterConversion) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  ConversionExecutors* conversion_executors =
      SubscriptionServiceFactory::GetInstance();
  DCHECK(conversion_executors);
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const auto filter_list_path = temp_dir.GetPath().AppendASCII("easylist.txt");
  std::vector<std::string_view> filter_list_contents = {
      "[\"Adblock Plus 2.0\"]\n", "invalid file"};
  for (const auto& file_content : filter_list_contents) {
    base::WriteFile(filter_list_path, file_content);
    ASSERT_TRUE(base::PathExists(filter_list_path));
    base::RunLoop run_loop;
    conversion_executors->ConvertFilterListFile(
        DefaultSubscriptionUrl(), filter_list_path,
        base::BindLambdaForTesting(
            [&run_loop](ConversionResult result) { run_loop.Quit(); }));
    run_loop.Run();
    ASSERT_FALSE(base::PathExists(filter_list_path));
  }
}

}  // namespace adblock
