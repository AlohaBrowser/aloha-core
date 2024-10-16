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

#include "base/base64.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "crypto/rsa_private_key.h"
#include "crypto/signature_creator.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockSitekeyTest : public AdblockBrowserTestBase {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule(kTestDomain, "127.0.0.1");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &AdblockSitekeyTest::RequestHandler, base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
    InitResourceClassificationObserver();
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (base::StartsWith(request.relative_url, "/test_page.html")) {
      static constexpr char kMainFrame[] =
          R"(
        <!DOCTYPE html>
        <html>
          <body>
            <iframe src="sitekey_iframe.html"></iframe>
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      if (add_doc_sitekey_) {
        http_response->AddCustomHeader(
            kSiteKeyHeaderKey, sitekey_publickey_ + "_" + sitekey_signature_);
      }
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kMainFrame);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    } else if (base::StartsWith(request.relative_url, "/sitekey_iframe.html")) {
      static constexpr char kIframe[] =
          R"(
        <!DOCTYPE html>
        <html>
          <body>
            <img src="/iframe_image.png" />
          </body>
        </html>)";
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      if (add_iframe_sitekey_) {
        http_response->AddCustomHeader(
            kSiteKeyHeaderKey, sitekey_publickey_ + "_" + sitekey_signature_);
      }
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kIframe);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  GURL GetPageUrl(const std::string& path = "/test_page.html") {
    return embedded_test_server()->GetURL(kTestDomain, path);
  }

  void NavigateToPage() {
    ASSERT_TRUE(content::NavigateToURL(shell(), GetPageUrl()));
  }

 protected:
  void CreateSitekey(const std::string& sitekey_uri) {
    std::string sitekey_ua =
        content::ShellContentBrowserClient::Get()->GetUserAgent();
    std::string sitekey_encryption_input =
        sitekey_uri + '\0' + kTestDomain + '\0' + sitekey_ua;
    std::unique_ptr<crypto::RSAPrivateKey> key_original(
        crypto::RSAPrivateKey::Create(1024));
    std::vector<uint8_t> priv_key;
    EXPECT_TRUE(key_original->ExportPrivateKey(&priv_key));
    std::vector<uint8_t> pub_key;
    EXPECT_TRUE(key_original->ExportPublicKey(&pub_key));
    std::unique_ptr<crypto::RSAPrivateKey> key(
        crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(priv_key));
    std::unique_ptr<crypto::SignatureCreator> signer(
        crypto::SignatureCreator::Create(key.get(),
                                         crypto::SignatureCreator::SHA1));
    EXPECT_TRUE(signer.get());
    EXPECT_TRUE(signer->Update(
        reinterpret_cast<const uint8_t*>(sitekey_encryption_input.c_str()),
        sitekey_encryption_input.size()));
    std::vector<uint8_t> signature;
    EXPECT_TRUE(signer->Final(&signature));
    sitekey_signature_ = base::Base64Encode(signature);
    sitekey_publickey_ = base::Base64Encode(pub_key);
  }

  std::string sitekey_signature_;
  std::string sitekey_publickey_;
  bool add_doc_sitekey_ = false;
  bool add_iframe_sitekey_ = false;
  static constexpr char kTestDomain[] = "test.org";
};

IN_PROC_BROWSER_TEST_F(AdblockSitekeyTest, VerifyIframeSitekey) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration);
  CreateSitekey("/sitekey_iframe.html");
  add_iframe_sitekey_ = true;
  SetFilters(
      {"iframe_image.png", "@@iframe_image.png$sitekey=" + sitekey_publickey_});
  NavigateToPage();
  ASSERT_EQ(observer_.allowed_ads_notifications.size(), 1u);
  EXPECT_TRUE(observer_.allowed_ads_notifications.front() ==
              GetPageUrl("/iframe_image.png"))
      << "Request not allowed!";
  EXPECT_TRUE(observer_.allowed_pages_notifications.empty());
  EXPECT_TRUE(observer_.blocked_ads_notifications.empty());
}

IN_PROC_BROWSER_TEST_F(AdblockSitekeyTest, VerifyDocumentSitekey) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(browser_context())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration);
  CreateSitekey("/test_page.html");
  add_doc_sitekey_ = true;
  SetFilters({"iframe_image.png",
              "@@iframe_image.png$sitekey=" + sitekey_publickey_,
              "@@test.org$document,sitekey=" + sitekey_publickey_});
  NavigateToPage();
  ASSERT_EQ(observer_.allowed_ads_notifications.size(), 1u);
  EXPECT_TRUE(observer_.allowed_ads_notifications.front() ==
              GetPageUrl("/iframe_image.png"))
      << "Request not allowed!";
  ASSERT_EQ(observer_.allowed_pages_notifications.size(), 1u);
  EXPECT_TRUE(observer_.allowed_pages_notifications.front() ==
              GetPageUrl("/test_page.html"))
      << "Request not allowed!";
  EXPECT_TRUE(observer_.blocked_ads_notifications.empty());
}

}  // namespace adblock
