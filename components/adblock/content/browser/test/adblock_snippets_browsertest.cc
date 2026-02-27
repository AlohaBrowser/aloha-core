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

#include <optional>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "components/adblock/content/browser/factories/snippet_service_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "components/adblock/core/snippet/snippet_service.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "crypto/obsolete/md5.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

namespace adblock {

namespace {
std::string Md5AsHexForTesting(base::span<const uint8_t> data) {
  auto hash = crypto::obsolete::Md5::HashForTesting(data);
  return base::ToLowerASCII(base::HexEncode(hash));
}
}  // namespace

class AdblockSnippetsBrowserTest : public AdblockBrowserTestBase {
 public:
  AdblockSnippetsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    SnippetServiceFactory::SetUpdateCheckIntervalForTesting(base::Seconds(1));
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdblockSnippetsBrowserTest::RequestHandler, base::Unretained(this)));
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {std::string(SnippetLibraryUrl().host())};
    https_server_.SetSSLConfig(cert_config);
    EXPECT_TRUE(https_server_.Start());
    OverrideSnippetLibraryPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetFilters(std::vector<std::string> filters) {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser_context())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    for (auto& filter : filters) {
      adblock_configuration->AddCustomFilter(filter);
    }
  }

  GURL GetUrl(const std::string& path) {
    return embedded_test_server()->GetURL("example.org", path);
  }

  void CheckAndCountGetRequestHeader(
      const net::test_server::HttpRequest& request) {
    const auto if_non_match_it =
        request.headers.find(net::HttpRequestHeaders::kIfNoneMatch);
    const bool has_if_none_match = if_non_match_it != request.headers.end();
    if (first_check_is_unconditional_ && get_requests_count_ == 0) {
      // A fresh profile has no entity-tag to revalidate with.
      EXPECT_FALSE(has_if_none_match);
    } else if (expected_if_none_match_) {
      // The stored entity-tag goes back exactly as the server issued it.
      if (has_if_none_match) {
        EXPECT_EQ(if_non_match_it->second, *expected_if_none_match_);
      } else {
        ADD_FAILURE() << "expected a conditional GET";
      }
    }
    ++get_requests_count_;
    get_requests_.emplace_back(has_if_none_match ? if_non_match_it->second
                                                 : "");
    if (!expected_get_requests_count_ ||
        get_requests_count_ == expected_get_requests_count_) {
      NotifyTestFinished();
    }
  }

  void VerifyTargetVisibility(bool is_hidden, const std::string& id) {
    std::string condition_js =
        "getComputedStyle(document.getElementById('{{node id}}')).display "
        "{{condition}} 'none'";
    base::ReplaceSubstringsAfterOffset(&condition_js, 0, "{{node id}}", id);
    base::ReplaceSubstringsAfterOffset(&condition_js, 0, "{{condition}}",
                                       is_hidden ? "==" : "!=");
    EXPECT_TRUE(WaitAndVerifyCondition(condition_js.c_str()));
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url.find("isolated-first") != std::string::npos) {
      if (request.method == net::test_server::HttpMethod::METHOD_HEAD) {
        // Counted, never expected: the library is revalidated with a
        // conditional GET. Tests assert this stays at zero.
        ++head_requests_count_;
        std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
            new net::test_server::BasicHttpResponse);
        http_response->set_code(net::HTTP_OK);
        http_response->set_content_type("text/plain");
        http_response->AddCustomHeader("ETag", get_etag_);
        return std::move(http_response);
      } else if (request.method == net::test_server::HttpMethod::METHOD_GET) {
        CheckAndCountGetRequestHeader(request);
        std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
            new net::test_server::BasicHttpResponse);
        http_response->set_code(net::HTTP_OK);
        http_response->set_content(snippets_library_);
        http_response->set_content_type("text/plain");
        http_response->AddCustomHeader("ETag", get_etag_);
        return std::move(http_response);
      }
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    // This is fine for the purpose of this test.
    return nullptr;
  }

  void SetCurrentEtag(const std::string& etag) {
    GetPrefs()->SetString(common::prefs::kSnippetsVersion, etag);
  }

  std::string GetSignature() {
    return GetPrefs()->GetString(common::prefs::kSnippetsSignature);
  }

  net::EmbeddedTestServer https_server_;
  std::string get_etag_;
  // When set, the entity-tag every conditional check must revalidate with.
  std::optional<std::string> expected_if_none_match_;
  // Set when the profile starts with no stored version.
  bool first_check_is_unconditional_ = false;
  std::string snippets_library_ =
      R"(// ! Version: 2.2.0
      (e, ...t) => {
      "console.log(\"snippets library\");
      };)";
  int head_requests_count_ = 0;
  int get_requests_count_ = 0;
  std::vector<std::string> get_requests_;
  int expected_get_requests_count_ = 0;
};

// A fresh profile has no entity-tag to revalidate with, so it fetches the
// library unconditionally and revalidates from the second check onwards. The
// entity-tag is quoted, as a real server issues it.
class AdblockSnippetsBrowserTest1stRunUpdate
    : public AdblockSnippetsBrowserTest {
 public:
  void SetUpOnMainThread() override {
    AdblockSnippetsBrowserTest::SetUpOnMainThread();
    get_etag_ = "\"get_etag_\"";
    expected_if_none_match_ = get_etag_;
    // Only the PRE_ run starts without a stored version; the run after it
    // reuses that profile and so revalidates from its first check.
    first_check_is_unconditional_ = base::StartsWith(
        ::testing::UnitTest::GetInstance()->current_test_info()->name(),
        "PRE_Test1stRunWithUpdate");
    expected_get_requests_count_ = 2;
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTest1stRunUpdate,
                       PRE_Test1stRunWithUpdate) {
  RunUntilTestFinished();
  EXPECT_EQ(0, head_requests_count_);
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTest1stRunUpdate,
                       Test1stRunWithUpdate) {
  EXPECT_EQ(SubscriptionValidatorImpl::ComputeSubscriptionHash(
                InMemoryFlatbufferData(snippets_library_)),
            GetSignature());
  EXPECT_EQ(SnippetServiceFactory::GetForBrowserContext(browser_context())
                ->GetSnippetVersion(),
            "2.2.0");
  EXPECT_EQ("\"get_etag_\"",
            GetPrefs()->GetString(common::prefs::kSnippetsVersion));
}

class AdblockSnippetsBrowserTestUpdate : public AdblockSnippetsBrowserTest {
 public:
  void SetUpOnMainThread() override {
    AdblockSnippetsBrowserTest::SetUpOnMainThread();
    if (base::StartsWith(
            ::testing::UnitTest::GetInstance()->current_test_info()->name(),
            "PRE_PRE_TestUpdateIfNewEtag")) {
      get_etag_ = "";
      SetCurrentEtag("\"old_etag\"");
    } else if (base::StartsWith(::testing::UnitTest::GetInstance()
                                    ->current_test_info()
                                    ->name(),
                                "PRE_TestUpdateIfNewEtag")) {
      get_etag_ = "\"get_etag_\"";
      expected_get_requests_count_ = 2;
    }
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdate,
                       PRE_PRE_TestUpdateIfNewEtag) {}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdate,
                       PRE_TestUpdateIfNewEtag) {
  RunUntilTestFinished();
  EXPECT_EQ(0, head_requests_count_);
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdate, TestUpdateIfNewEtag) {
  EXPECT_EQ(SubscriptionValidatorImpl::ComputeSubscriptionHash(
                InMemoryFlatbufferData(snippets_library_)),
            GetSignature());
  EXPECT_EQ(SnippetServiceFactory::GetForBrowserContext(browser_context())
                ->GetSnippetVersion(),
            "2.2.0");
}

// FIXME: Remove or enable depending on DPD-3415
class AdblockSnippetsBrowserTestUpdateBrokenLibrary
    : public AdblockSnippetsBrowserTestUpdate {
 public:
  void SetUpOnMainThread() override {
    AdblockSnippetsBrowserTestUpdate::SetUpOnMainThread();
    snippets_library_ = "&^%^$$%$^%$";
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdateBrokenLibrary,
                       DISABLED_PRE_PRE_TestUpdateIfNewEtag) {}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdateBrokenLibrary,
                       DISABLED_PRE_TestUpdateIfNewEtag) {
  RunUntilTestFinished();
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdateBrokenLibrary,
                       DISABLED_TestUpdateIfNewEtag) {
  // Signature is empty when no update was stored
  EXPECT_TRUE(GetSignature().empty());
}

class AdblockSnippetsBrowserTestNoUpdate : public AdblockSnippetsBrowserTest {
 public:
  void SetUpOnMainThread() override {
    AdblockSnippetsBrowserTest::SetUpOnMainThread();
    auto resource_data =
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            IDR_ADBLOCK_SNIPPETS_JS);
    auto etag =
        "\"" + Md5AsHexForTesting(base::as_byte_span(resource_data)) + "\"";
    if (base::StartsWith(
            ::testing::UnitTest::GetInstance()->current_test_info()->name(),
            "PRE_PRE_TestNoUpdateIfNoNewEtag")) {
      SetCurrentEtag(etag);
    } else if (base::StartsWith(::testing::UnitTest::GetInstance()
                                    ->current_test_info()
                                    ->name(),
                                "PRE_TestNoUpdateIfNoNewEtag")) {
      get_etag_ = etag;
      expected_if_none_match_ = etag;
      expected_get_requests_count_ = 2;
    }
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestNoUpdate,
                       PRE_PRE_TestNoUpdateIfNoNewEtag) {}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestNoUpdate,
                       PRE_TestNoUpdateIfNoNewEtag) {
  RunUntilTestFinished();
  EXPECT_EQ(0, head_requests_count_);
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestNoUpdate,
                       TestNoUpdateIfNoNewEtag) {
  // Signature is empty when no update was stored
  EXPECT_TRUE(GetSignature().empty());
}

class AdblockSnippetsBrowserTestUpdateDisabled
    : public AdblockSnippetsBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        adblock::switches::kDisableEyeoSnippetsLibraryUpdate);
  }

  void VerifyNoDownloads() {
    ASSERT_EQ(0u, head_requests_count_);
    ASSERT_EQ(0u, get_requests_count_);
    NotifyTestFinished();
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTestUpdateDisabled,
                       TestNoDownloads) {
  // This test assumes that inital downloads will happen within 10 seconds.
  base::OneShotTimer timer;
  timer.Start(FROM_HERE, base::Seconds(10),
              base::BindOnce(
                  &AdblockSnippetsBrowserTestUpdateDisabled::VerifyNoDownloads,
                  base::Unretained(this)));
  RunUntilTestFinished();
}

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTest, VerifyXpath3) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl("/xpath3.html")));
  VerifyTargetVisibility(false, "xpath3-target");
  SetFilters(
      {"example.org#$#hide-if-matches-xpath3 //*[@id=\"xpath3-target\"]"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl("/xpath3.html")));
  VerifyTargetVisibility(true, "xpath3-target");
}

}  // namespace adblock
