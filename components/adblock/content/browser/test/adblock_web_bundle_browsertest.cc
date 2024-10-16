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

#include <cstdint>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/web_package/web_bundle_builder.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockWebBundleBrowserTest
    : public AdblockBrowserTestBase,
      public ResourceClassificationRunner::Observer {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->RegisterDefaultHandler(
        base::BindRepeating(&AdblockWebBundleBrowserTest::HandleFileRequest,
                            base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
    PrepareTempDirWithContent();
    ResourceClassificationRunnerFactory::GetForBrowserContext(browser_context())
        ->AddObserver(this);
  }

  void TearDownOnMainThread() override {
    ResourceClassificationRunnerFactory::GetForBrowserContext(browser_context())
        ->RemoveObserver(this);
    AdblockBrowserTestBase::TearDownOnMainThread();
  }

  void PrepareTempDirWithContent() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    CreateIndexFile();
    CreateByResourceWebBundle();
    CreateByBundleFileWebBundle();
    CreateByScopeFileWebBundle();
  }

  std::string GetFileContentFromTestDir(
      base::FilePath::StringType relative_path) {
    base::FilePath root;
    CHECK(base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &root));
    root = root.AppendASCII("components/test/data/adblock/wbn");
    std::string content;
    CHECK(base::ReadFileToString(root.Append(relative_path), &content));
    return content;
  }

  // In order for an html file to use absolute URLs in 'src' attributes to
  // resources hosted by the embedded_test_server(), we need to replace
  // {{{baseUrl}}} with the server's URL. It changes for every run.
  std::string GetHtmlContentWithReplacements(
      base::FilePath::StringType relative_path) {
    std::string content = GetFileContentFromTestDir(relative_path);
    base::ReplaceSubstringsAfterOffset(
        &content, 0, "{{{baseUrl}}}",
        embedded_test_server()->GetURL("example.org", "/").spec());
    return content;
  }

  void CreateByResourceWebBundle() {
    web_package::WebBundleBuilder builder;
    builder.AddExchange("by_resource/blue_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_resource/blue_subresource_loading.css")));
    builder.AddExchange("by_resource/blue_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_resource/blue_subresource_loading.png")));
    builder.AddExchange("by_resource/red_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_resource/red_subresource_loading.css")));
    builder.AddExchange("by_resource/red_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_resource/red_subresource_loading.png")));
    builder.AddExchange(
        "by_resource/xhr_result_1_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_resource/xhr_result_1_subresource_loading.json")));
    builder.AddExchange(
        "by_resource/fetch_result_1_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_resource/fetch_result_1_subresource_loading.json")));
    builder.AddPrimaryURL(embedded_test_server()->GetURL("example.org", "/"));
    const auto binary_data = builder.CreateBundle();
    ASSERT_TRUE(base::WriteFile(
        temp_dir_.GetPath().AppendASCII("by_resource.wbn"), binary_data));
  }

  void CreateByBundleFileWebBundle() {
    web_package::WebBundleBuilder builder;
    builder.AddExchange("by_bundle_file/green_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_bundle_file/green_subresource_loading.css")));
    builder.AddExchange("by_bundle_file/green_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_bundle_file/green_subresource_loading.png")));
    builder.AddExchange("by_bundle_file/purple_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_bundle_file/purple_subresource_loading.css")));
    builder.AddExchange("by_bundle_file/purple_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_bundle_file/purple_subresource_loading.png")));
    builder.AddExchange(
        "by_bundle_file/xhr_result_2_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_bundle_file/xhr_result_2_subresource_loading.json")));
    builder.AddExchange(
        "by_bundle_file/fetch_result_2_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_bundle_file/fetch_result_2_subresource_loading.json")));
    builder.AddPrimaryURL(embedded_test_server()->GetURL("example.org", "/"));
    const auto binary_data = builder.CreateBundle();
    ASSERT_TRUE(base::WriteFile(
        temp_dir_.GetPath().AppendASCII("by_bundle_file.wbn"), binary_data));
  }

  void CreateByScopeFileWebBundle() {
    web_package::WebBundleBuilder builder;
    builder.AddExchange("by_scope/orange_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_scope/orange_subresource_loading.css")));
    builder.AddExchange("by_scope/orange_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_scope/orange_subresource_loading.png")));
    builder.AddExchange("by_scope/pink_subresource_loading.css",
                        {{":status", "200"}, {"content-type", "text/css"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_scope/pink_subresource_loading.css")));
    builder.AddExchange("by_scope/pink_subresource_loading.png",
                        {{":status", "200"}, {"content-type", "image/png"}},
                        GetFileContentFromTestDir(FILE_PATH_LITERAL(
                            "by_scope/pink_subresource_loading.png")));
    builder.AddExchange(
        "by_scope/xhr_result_3_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_scope/xhr_result_3_subresource_loading.json")));
    builder.AddExchange(
        "by_scope/fetch_result_3_subresource_loading.json",
        {{":status", "200"}, {"content-type", "application/json"}},
        GetFileContentFromTestDir(FILE_PATH_LITERAL(
            "by_scope/fetch_result_3_subresource_loading.json")));
    builder.AddPrimaryURL(embedded_test_server()->GetURL("example.org", "/"));
    const auto binary_data = builder.CreateBundle();
    ASSERT_TRUE(base::WriteFile(temp_dir_.GetPath().AppendASCII("by_scope.wbn"),
                                binary_data));
  }

  void CreateIndexFile() {
    const auto html = GetHtmlContentWithReplacements(
        FILE_PATH_LITERAL("index.html.mustache"));
    ASSERT_TRUE(
        base::WriteFile(temp_dir_.GetPath().AppendASCII("index.html"), html));
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleFileRequest(
      const net::test_server::HttpRequest& request) {
    auto response =
        net::test_server::HandleFileRequest(temp_dir_.GetPath(), request);
    if (response) {
      auto* basic =
          static_cast<net::test_server::BasicHttpResponse*>(response.get());
      if (temp_dir_.GetPath()
              .AppendASCII(request.GetURL().path().substr(1))
              .MatchesExtension(FILE_PATH_LITERAL(".wbn"))) {
        basic->set_content_type("application/webbundle");
      }
      basic->AddCustomHeader("X-Content-Type-Options", "nosniff");
      basic->AddCustomHeader("Access-Control-Allow-Origin", "*");
    }
    return response;
  }

  GURL GetUrl() {
    return embedded_test_server()->GetURL("example.org", "/index.html");
  }

  bool EvaluateJs(std::string value) {
    return content::EvalJs(web_contents(), value).ExtractBool();
  }

  bool IsImageLoaded(std::string selector) {
    return EvaluateJs(
        "(function(){ node = document.querySelector('" + selector +
        "'); return node.complete && node.naturalHeight !== 0; })()");
  }

  bool IsCssBlocked(std::string selector) {
    // On the test page, a box with this |id| is grey if element-specific CSS
    // was blocked. Grey is the default color for divs, and gets overridden
    // to element-specific colors by blockable CSS files.
    return EvaluateJs(
        "(function(){ return "
        "window.getComputedStyle(document.querySelector('" +
        selector + "')).backgroundColor == 'rgb(128, 128, 128)'; })()");
  }

  // Waits until the textContent of the selected item stops being "Pending" and
  // returns whatever it's final value is.
  // This asynchronous wait is because scripts that change the textContent may
  // execute with a delay.
  std::string GetSelectorTextContent(std::string selector) {
    return content::EvalJs(web_contents(),
                           R"(
 (async function (selector) {
    return new Promise(resolve => {
        if (!document.querySelector(selector).textContent.includes('Pending')) {
            return resolve(document.querySelector(selector).textContent);
        }

        const observer = new MutationObserver(mutations => {
            if (!document.querySelector(selector).textContent.includes('Pending')) {
                resolve(document.querySelector(selector).textContent);
                observer.disconnect();
            }
        });

        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
    });
})(' )" + selector + "');")
        .ExtractString();
  }

  bool IsXhrOrFetchRequestBlocked(std::string selector) {
    return GetSelectorTextContent(selector).find("Error") != std::string::npos;
  }

  bool IsXhrOrFetchRequestAllowed(std::string selector) {
    return GetSelectorTextContent(selector).find("succeeded") !=
           std::string::npos;
  }

  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {
    if (match_result == FilterMatchResult::kBlockRule) {
      if (!blocked_ads_expectations_.empty()) {
        blocked_ads_expectations_.erase(
            base::ranges::remove(blocked_ads_expectations_, url),
            blocked_ads_expectations_.end());
        if (blocked_ads_expectations_.empty()) {
          NotifyTestFinished();
        }
      } else {
        blocked_ads_notifications_.push_back(url);
      }
    }
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {}

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {}

  base::ScopedTempDir temp_dir_;
  // Note: Use either blocked_ads_notifications_ or blocked_ads_expectations_
  std::vector<GURL> blocked_ads_notifications_;
  std::vector<GURL> blocked_ads_expectations_;
};

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockImageByResource) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  const auto* blocked_image_selector =
      R"(img[src="by_resource/blue_subresource_loading.png"])";
  const auto* other_image_selector =
      R"(img[src="by_resource/red_subresource_loading.png"])";
  EXPECT_TRUE(IsImageLoaded(blocked_image_selector));
  EXPECT_TRUE(IsImageLoaded(other_image_selector));
  SetFilters({"/blue_subresource_loading.png"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_FALSE(IsImageLoaded(blocked_image_selector));
  EXPECT_TRUE(IsImageLoaded(other_image_selector));
}

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockCssByResource) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_FALSE(IsCssBlocked("div.blue"));
  SetFilters({"blue_subresource_loading.css"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_TRUE(IsCssBlocked("div.blue"));
}

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockXhrByResource) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  const auto* blocked_selector = "code#xhr_result_by_resource";
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed(blocked_selector));
  SetFilters({"/xhr_result_1_subresource_loading.json"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_TRUE(IsXhrOrFetchRequestBlocked(blocked_selector));
}

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockFetchByResource) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  const auto* blocked_selector = "code#fetch_result_by_resource";
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed(blocked_selector));
  SetFilters({"/fetch_result_1_subresource_loading.json"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_TRUE(IsXhrOrFetchRequestBlocked(blocked_selector));
}

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockByBundle) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  const auto* blocked_image_selector =
      R"(img[src="by_bundle_file/green_subresource_loading.png"])";
  EXPECT_TRUE(IsImageLoaded(blocked_image_selector));
  EXPECT_FALSE(IsCssBlocked("div.green"));
  EXPECT_FALSE(IsCssBlocked("div.purple"));
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed("code#xhr_result_by_bundle_file"));
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed("code#fetch_result_by_bundle_file"));

  SetFilters({"by_bundle_file.wbn$webbundle"});
  // DPD-2689: Sometimes 2nd navigation stuck and test timeouts due to lack
  // of propagation that image(s) within a bundle were blocked (by blocking
  // whole bundle). To workaround this timing related upstream issue (as it
  // seems) instead of calling blocking NavigateToURL() we just commit
  // navigation directly and wait for expected request blocked event and
  // document ready state before we proceed with subresources verification.
  blocked_ads_expectations_.push_back(
      embedded_test_server()->GetURL("example.org", "/by_bundle_file.wbn"));
  web_contents()->GetController().LoadURL(
      GetUrl(), content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
  RunUntilTestFinished();
  WaitAndVerifyCondition("document.readyState === 'complete'");
  EXPECT_FALSE(IsImageLoaded(blocked_image_selector));
  EXPECT_TRUE(IsCssBlocked("div.green"));
  EXPECT_TRUE(IsCssBlocked("div.purple"));
  // FIXME the following two expectations fail because the state of the requests
  // is "pending" rather than allowed or blocked: DPD-1887
  // EXPECT_TRUE(IsXhrOrFetchRequestBlocked("code#xhr_result_by_bundle_file"));
  // EXPECT_TRUE(IsXhrOrFetchRequestBlocked("code#fetch_result_by_bundle_file"));
}

IN_PROC_BROWSER_TEST_F(AdblockWebBundleBrowserTest, BlockByScope) {
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  const auto* blocked_image_selector =
      R"(img[src="by_scope/orange_subresource_loading.png"])";
  EXPECT_TRUE(IsImageLoaded(blocked_image_selector));
  EXPECT_FALSE(IsCssBlocked("div.orange"));
  EXPECT_FALSE(IsCssBlocked("div.pink"));
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed("code#xhr_result_by_scope"));
  EXPECT_TRUE(IsXhrOrFetchRequestAllowed("code#fetch_result_by_scope"));

  SetFilters({"by_scope/"});
  ASSERT_TRUE(content::NavigateToURL(shell(), GetUrl()));
  EXPECT_FALSE(IsImageLoaded(blocked_image_selector));
  EXPECT_TRUE(IsCssBlocked("div.orange"));
  EXPECT_TRUE(IsCssBlocked("div.pink"));
  EXPECT_TRUE(IsXhrOrFetchRequestBlocked("code#xhr_result_by_scope"));
  EXPECT_TRUE(IsXhrOrFetchRequestBlocked("code#fetch_result_by_scope"));
}

// TODO:
// - Mixed origins
// - Signed bundles. including case of navigation between signed and unsigned
// content.
// - If you block the whole bundle, all resources requests to components
// declared in <script type="webbundle"> should be aborted. See sample test
// here:
// https://gitlab.com/eyeo/adblockplus/abc/webext-sdk/-/merge_requests/623/diffs#c24c6bdc211dc8e196dc5e2e32ddbefeb5fd4390_179_179

}  // namespace adblock
