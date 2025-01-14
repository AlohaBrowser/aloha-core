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

#include <vector>

#include "absl/strings/str_format.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockContentFiltersBrowserTest : public AdblockBrowserTestBase {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("example.com", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  GURL GetUrl(const std::string& path) {
    return embedded_test_server()->GetURL("example.com", path);
  }

  void WaitForDynamicContentLoaded() {
    std::string dynamic_content_loaded =
        "window.dynamic_content_loaded == true";
    ASSERT_TRUE(WaitAndVerifyCondition(dynamic_content_loaded.c_str()));
  }

  void VerifyTargetsRemoved(bool removed, const std::string& class_id) {
    std::string is_removed_js =
        base::StringPrintf("document.getElementsByClassName('%s').length == %d",
                           class_id.c_str(), removed ? 0 : 2);
    EXPECT_TRUE(WaitAndVerifyCondition(is_removed_js.c_str()));
  }

  void VerifyTargetHidden(bool hidden, const std::string& id) {
    std::string expected_visibility = (hidden ? "none" : "inline");
    std::string is_hidden_js = base::StringPrintf(
        "window.getComputedStyle(document.getElementById('%s'))."
        "display == '%s'",
        id.c_str(), expected_visibility.c_str());
    EXPECT_TRUE(WaitAndVerifyCondition(is_hidden_js.c_str()));
  }

  void VerifyTargetsHidden(bool hidden, const std::string& class_id) {
    std::string expected_visibility = (hidden ? "none" : "inline");
    std::string is_hidden_js = base::StringPrintf(
        "window.getComputedStyle(document.getElementsByClassName('%s')[0])."
        "display == '%s' && "
        "window.getComputedStyle(document.getElementsByClassName('%s')[1])."
        "display == '%s'",
        class_id.c_str(), expected_visibility.c_str(), class_id.c_str(),
        expected_visibility.c_str());
    EXPECT_TRUE(WaitAndVerifyCondition(is_hidden_js.c_str()));
  }

  void VerifyCssAppliedForTarget(bool applied, const std::string& id) {
    std::string expected_css = (applied ? "rgb(0, 255, 0)" : "rgb(255, 0, 0)");
    std::string is_css_applied_js = base::StringPrintf(
        "window.getComputedStyle(document.getElementById('%s'))."
        "backgroundColor == '%s'",
        id.c_str(), expected_css.c_str());
    EXPECT_TRUE(WaitAndVerifyCondition(is_css_applied_js.c_str()));
  }

  void VerifyCssAppliedForTargets(bool applied, const std::string& class_id) {
    std::string expected_css = (applied ? "rgb(0, 255, 0)" : "rgb(255, 0, 0)");
    std::string is_css_applied_js = base::StringPrintf(
        "window.getComputedStyle(document.getElementsByClassName('%s')[0])."
        "backgroundColor == '%s' && "
        "window.getComputedStyle(document.getElementsByClassName('%s')[1])."
        "backgroundColor == '%s'",
        class_id.c_str(), expected_css.c_str(), class_id.c_str(),
        expected_css.c_str());
    EXPECT_TRUE(WaitAndVerifyCondition(is_css_applied_js.c_str()));
  }
};

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyNoFilters) {
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyHide) {
  SetFilters({"example.com##.id_to_elem_hide",
              "example.com#?#span:-abp-contains(id_to_elem_hide_emu)"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(true, "id_to_elem_hide");
  VerifyTargetsHidden(true, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyHideException) {
  SetFilters({"example.com##.id_to_elem_hide",
              "example.com#?#span:-abp-contains(id_to_elem_hide_emu)",
              "example.com#@#.id_to_elem_hide",
              "example.com#@#span:-abp-contains(id_to_elem_hide_emu)"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyRemove) {
  SetFilters({"example.com##.id_to_remove_by_eh {remove: true;}",
              "example.com#?#span:-abp-contains(id_to_remove_by_ehe) {remove: "
              "true;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(true, "id_to_remove_by_eh");
  VerifyTargetsRemoved(true, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest,
                       VerifyRemoveException) {
  SetFilters({"example.com##.id_to_remove_by_eh {remove: true;}",
              "example.com#?#span:-abp-contains(id_to_remove_by_ehe\"]"
              ") {remove: true;}",
              "example.com#@#.id_to_remove_by_eh",
              "example.com#@#span:-abp-contains(id_to_remove_by_ehe)"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyInlineCss) {
  SetFilters(
      {"example.com##.id_to_apply_style_by_eh {background-color: "
       "#00FF00!important;}",
       "example.com#?#span:-abp-contains(id_to_apply_style_by_ehe) "
       "{background-color: #00FF00!important;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(true, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(true, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest,
                       VerifyInlineCssException) {
  SetFilters(
      {"example.com##.id_to_apply_style_by_eh {background-color: "
       "#00FF00!important;}",
       "example.com#?#span:-abp-contains(id_to_apply_style_by_ehe) "
       "{background-color: #00FF00!important;}",
       "example.com#@#.id_to_apply_style_by_eh",
       "example.com#@#span:-abp-contains(id_to_apply_style_by_ehe)"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(false, "id_to_elem_hide");
  VerifyTargetsHidden(false, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(false, "id_to_remove_by_eh");
  VerifyTargetsRemoved(false, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(false, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest, VerifyAllFilters) {
  SetFilters({"example.com##.id_to_elem_hide",
              "example.com#?#span:-abp-contains(id_to_elem_hide_emu)",
              "example.com##.id_to_remove_by_eh {remove: true;}",
              "example.com#?#span:-abp-contains(id_to_remove_by_ehe) {"
              "remove: true;}",
              "example.com##.id_to_apply_style_by_eh {background-color: "
              "#00FF00!important;}",
              "example.com#?#span:-abp-contains(id_to_apply_style_by_ehe) "
              "{background-color: #00FF00!important;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  WaitForDynamicContentLoaded();
  VerifyTargetsHidden(true, "id_to_elem_hide");
  VerifyTargetsHidden(true, "id_to_elem_hide_emu");
  VerifyTargetsRemoved(true, "id_to_remove_by_eh");
  VerifyTargetsRemoved(true, "id_to_remove_by_ehe");
  VerifyCssAppliedForTargets(true, "id_to_apply_style_by_eh");
  VerifyCssAppliedForTargets(true, "id_to_apply_style_by_ehe");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest,
                       VerifyHideToInlineCssSelectorChange) {
  SetFilters({"example.com#?#span:-abp-contains(hide_selector)",
              "example.com#?#span:-abp-contains(inline_css_selector) "
              "{background-color: #00FF00!important;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  VerifyTargetHidden(false, "changing_element");
  VerifyCssAppliedForTarget(false, "changing_element");
  EXPECT_EQ(
      "hide_selector",
      content::EvalJs(web_contents(),
                      "document.getElementById('changing_element').innerHTML = "
                      "'hide_selector'"));
  VerifyTargetHidden(true, "changing_element");
  VerifyCssAppliedForTarget(false, "changing_element");
  EXPECT_EQ(
      "inline_css_selector",
      content::EvalJs(web_contents(),
                      "document.getElementById('changing_element').innerHTML = "
                      "'inline_css_selector'"));
  VerifyTargetHidden(false, "changing_element");
  VerifyCssAppliedForTarget(true, "changing_element");
}

IN_PROC_BROWSER_TEST_F(AdblockContentFiltersBrowserTest,
                       VerifyInlineCssStyleModificationLogic) {
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  static constexpr char get_expected_style_property[] =
      "document.getElementById('changing_element').style['%s'] === '%s'";
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "background-color",
                         "rgb(255, 0, 0)")
          .c_str()));
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "width", "").c_str()));
  SetFilters(
      {"example.com###changing_element {background-color: #00FF00!important;}",
       "example.com###changing_element {width: 100px;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  // "background-color" is now overwritten (update logic for existing property)
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "background-color",
                         "rgb(0, 255, 0)")
          .c_str()));
  // "width" is now set (add logic for not yet set property)
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "width", "100px")
          .c_str()));
}

}  // namespace adblock
