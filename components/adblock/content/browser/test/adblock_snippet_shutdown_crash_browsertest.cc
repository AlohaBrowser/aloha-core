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

#include "base/run_loop.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/threading/thread_restrictions.h"
#include "components/adblock/content/browser/factories/snippet_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/snippet/snippet_service.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockSnippetShutdownCrashBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockSnippetShutdownCrashBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    SnippetServiceFactory::SetUpdateCheckIntervalForTesting(
        base::Milliseconds(100));
    SnippetServiceFactory::SetInitialDelayForTesting(base::Milliseconds(1));

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdblockSnippetShutdownCrashBrowserTest::SlowResponseHandler,
        base::Unretained(this)));

    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {std::string(SnippetLibraryUrl().host())};
    https_server_.SetSSLConfig(cert_config);
    EXPECT_TRUE(https_server_.Start());
    OverrideSnippetLibraryPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  std::unique_ptr<net::test_server::HttpResponse> SlowResponseHandler(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url.find("isolated-first") != std::string::npos) {
      request_received_ = true;

      if (request_received_callback_) {
        std::move(request_received_callback_).Run();
      }

      if (request.method == net::test_server::HttpMethod::METHOD_HEAD) {
        auto response = std::make_unique<net::test_server::DelayedHttpResponse>(
            base::Seconds(5));  // Long delay
        response->set_code(net::HTTP_OK);
        response->set_content_type("text/plain");
        response->AddCustomHeader("ETag", "test_etag_v1.0.0");
        return response;
      } else if (request.method == net::test_server::HttpMethod::METHOD_GET) {
        auto response = std::make_unique<net::test_server::DelayedHttpResponse>(
            base::Seconds(5));  // Long delay
        response->set_code(net::HTTP_OK);
        response->set_content(snippets_library_content_);
        response->set_content_type("text/plain");
        response->AddCustomHeader("ETag", "test_etag_v1.0.0");
        return response;
      }
    }
    return nullptr;
  }

  void WaitForRequestToStart(base::OnceClosure callback) {
    request_received_callback_ = std::move(callback);
  }

  bool RequestWasReceived() const { return request_received_; }

 protected:
  net::EmbeddedTestServer https_server_;
  std::string snippets_library_content_ =
      R"(// ! Version: 1.0.0
      (e, ...t) => {
        console.log("test snippet");
      };)";
  bool request_received_ = false;
  base::OnceClosure request_received_callback_;
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetShutdownCrashBrowserTest,
                       CrashOnShutdownDuringHeadRequest) {
  GetPrefs()->ClearPref(common::prefs::kSnippetsVersion);
  GetPrefs()->ClearPref(common::prefs::kSnippetsSignature);

  base::RunLoop wait_for_request;
  WaitForRequestToStart(wait_for_request.QuitClosure());

  auto* snippet_service =
      SnippetServiceFactory::GetForBrowserContext(browser_context());
  ASSERT_NE(snippet_service, nullptr);

  wait_for_request.Run();
  ASSERT_TRUE(RequestWasReceived());

  TearDownOnMainThread();
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetShutdownCrashBrowserTest,
                       CrashOnShutdownDuringGetRequest) {
  GetPrefs()->SetString(common::prefs::kSnippetsVersion, "old_version");

  base::RunLoop wait_for_request;
  WaitForRequestToStart(wait_for_request.QuitClosure());

  auto* snippet_service =
      SnippetServiceFactory::GetForBrowserContext(browser_context());
  ASSERT_NE(snippet_service, nullptr);

  wait_for_request.Run();
  ASSERT_TRUE(RequestWasReceived());

  TearDownOnMainThread();
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetShutdownCrashBrowserTest,
                       CrashOnRapidShutdown) {
  GetPrefs()->ClearPref(common::prefs::kSnippetsVersion);

  auto* snippet_service =
      SnippetServiceFactory::GetForBrowserContext(browser_context());
  ASSERT_NE(snippet_service, nullptr);

  TearDownOnMainThread();
}

}  // namespace adblock
