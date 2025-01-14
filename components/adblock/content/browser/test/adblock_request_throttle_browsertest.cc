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

#include "components/adblock/core/net/adblock_request_throttle.h"

#include "base/check.h"
#include "base/functional/callback_forward.h"
#include "base/time/time.h"
#include "components/adblock/content/browser/factories/adblock_request_throttle_factory.h"
#include "components/adblock/content/browser/factories/adblock_telemetry_service_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/activeping_telemetry_topic_provider.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "gtest/gtest.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockRequestThrottleBrowsertest : public AdblockBrowserTestBase {
 public:
  AdblockRequestThrottleBrowsertest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    auto request_handler =
        base::BindRepeating(&AdblockRequestThrottleBrowsertest::RequestHandler,
                            base::Unretained(this));
    https_server_.RegisterRequestHandler(request_handler);
    // Filter list requests and recommendations.json are made over HTTPS.
    // All of them target the same host, so we can use the same certificate.
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org",
                             GetTelemetryDomain()};
    https_server_.SetSSLConfig(cert_config);
    EXPECT_TRUE(https_server_.Start());
    SetFilterListServerPortForTesting(https_server_.port());

    ActivepingTelemetryTopicProvider::SetHttpsPortForTesting(
        https_server_.port());
    // Make sure telemetry pings are sent often enough for our test to register
    // one.
    const auto testing_interval = base::Seconds(2);
    ActivepingTelemetryTopicProvider::SetIntervalsForTesting(testing_interval);
    AdblockTelemetryServiceFactory::GetInstance()->SetCheckIntervalForTesting(
        testing_interval);
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    // Delay all network requests by 5 seconds (instead of the default 30, to
    // make the test finish within the timeout).
    AdblockRequestThrottleFactory::GetForBrowserContext(browser_context())
        ->AllowRequestsAfter(base::Seconds(5));
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    // We expect 4 filter list downloads.
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(request.relative_url, "/easylist.txt") ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt") ||
         base::StartsWith(request.relative_url, "/recommendations.json"))) {
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }
    // We also expect an activeping request.
    if (request.method == net::test_server::HttpMethod::METHOD_POST &&
        base::StartsWith(request.relative_url,
                         "/topic/eyeochromium_activeping")) {
      activeping_request_received_ = true;
    }
    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (CheckExpectedDownloads()) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    // This is fine for the purpose of this test.
    return nullptr;
  }

  bool CheckExpectedDownloads() {
    return default_lists_.size() == 4 && activeping_request_received_;
  }

 protected:
  net::EmbeddedTestServer https_server_;
  std::set<std::string> default_lists_;
  bool activeping_request_received_ = false;
  bool finish_condition_met_ = false;
  base::RepeatingClosure quit_closure_;
};

IN_PROC_BROWSER_TEST_F(AdblockRequestThrottleBrowsertest,
                       AllInitialDownloadsAllowedEventually) {
  // Runs untill all expected initial network requests are made:
  // - default filter lists
  // - activeping telemetry
  // - recommendations.json
  // This will "hang" for 5 seconds intentionally, esuring the request throttler
  // does its job.
  RunUntilTestFinished();
}

}  // namespace adblock
