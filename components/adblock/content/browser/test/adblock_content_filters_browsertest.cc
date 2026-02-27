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
#include "base/strings/stringprintf.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockContentFiltersBrowserTest
    : public AdblockBrowserTestBase,
      public testing::WithParamInterface<bool> {
 public:
  void SetUpOnMainThread() override {
    AdblockBrowserTestBase::SetUpOnMainThread();
    host_resolver()->AddRule("example.com", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "components/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
    InitResourceClassificationObserver();
  }

  bool IncognitoMode() override { return GetParam(); }

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

  void VerifyTargetRemoved(bool removed, const std::string& id) {
    std::string is_removed_js =
        base::StringPrintf("!!document.getElementById('%s') === %s", id.c_str(),
                           removed ? "false" : "true");
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

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyNoFilters) {
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

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyHide) {
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
  ASSERT_EQ(observer_.hidden_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.hidden_notifications_,
      testing::UnorderedElementsAre(
          "type:span;className:id_to_elem_hide",
          ":root > BODY:nth-child(2) > DIV:nth-child(3) > SPAN:nth-child(1)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyHideException) {
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
  EXPECT_EQ(observer_.hidden_notifications_.size(), 0u);
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyRemove) {
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
  ASSERT_EQ(observer_.removed_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.removed_notifications_,
      testing::UnorderedElementsAre(
          ".id_to_remove_by_eh",
          ":root > BODY:nth-child(2) > DIV:nth-child(5) > SPAN:nth-child(1)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest,
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
  EXPECT_EQ(observer_.removed_notifications_.size(), 0u);
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyInlineCss) {
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
  ASSERT_EQ(observer_.inline_css_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.inline_css_notifications_,
      testing::UnorderedElementsAre(
          ".id_to_apply_style_by_eh",
          ":root > BODY:nth-child(2) > DIV:nth-child(7) > SPAN:nth-child(1)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest,
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
  EXPECT_EQ(observer_.inline_css_notifications_.size(), 0u);
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest, VerifyAllFilters) {
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
  ASSERT_EQ(observer_.hidden_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.hidden_notifications_,
      testing::UnorderedElementsAre(
          "type:span;className:id_to_elem_hide",
          ":root > BODY:nth-child(2) > DIV:nth-child(3) > SPAN:nth-child(1)"));
  ASSERT_EQ(observer_.removed_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.removed_notifications_,
      testing::UnorderedElementsAre(
          ".id_to_remove_by_eh",
          ":root > BODY:nth-child(2) > DIV:nth-child(5) > SPAN:nth-child(1)"));
  ASSERT_EQ(observer_.inline_css_notifications_.size(), 2u);
  EXPECT_THAT(
      observer_.inline_css_notifications_,
      testing::UnorderedElementsAre(
          ".id_to_apply_style_by_eh",
          ":root > BODY:nth-child(2) > DIV:nth-child(7) > SPAN:nth-child(1)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest,
                       VerifyHideToInlineCssSelectorChange) {
  SetFilters({"example.com#?#span:-abp-contains(hide_selector)",
              "example.com#?#span:-abp-contains(inline_css_selector) "
              "{background-color: #00FF00!important;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  VerifyTargetHidden(false, "hidden_then_inline_css");
  VerifyCssAppliedForTarget(false, "hidden_then_inline_css");
  EXPECT_EQ("hide_selector",
            content::EvalJs(
                web_contents(),
                "document.getElementById('hidden_then_inline_css').innerHTML = "
                "'hide_selector'"));
  VerifyTargetHidden(true, "hidden_then_inline_css");
  VerifyCssAppliedForTarget(false, "hidden_then_inline_css");
  EXPECT_EQ("inline_css_selector",
            content::EvalJs(
                web_contents(),
                "document.getElementById('hidden_then_inline_css').innerHTML = "
                "'inline_css_selector'"));
  VerifyTargetHidden(false, "hidden_then_inline_css");
  VerifyCssAppliedForTarget(true, "hidden_then_inline_css");
  ASSERT_EQ(observer_.hidden_notifications_.size(), 1u);
  EXPECT_THAT(
      observer_.hidden_notifications_,
      testing::UnorderedElementsAre(
          ":root > BODY:nth-child(2) > DIV:nth-child(11) > SPAN:nth-child(2)"));
  ASSERT_EQ(observer_.inline_css_notifications_.size(), 1u);
  EXPECT_THAT(
      observer_.inline_css_notifications_,
      testing::UnorderedElementsAre(
          ":root > BODY:nth-child(2) > DIV:nth-child(11) > SPAN:nth-child(2)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest,
                       VerifyHideToRemoveSelectorChange) {
  SetFilters(
      {"example.com#?#span:-abp-contains(hide_selector)",
       "example.com#?#span:-abp-contains(remove_selector) {remove: true;}"});
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  VerifyTargetHidden(false, "hidden_then_remove");
  VerifyTargetRemoved(false, "hidden_then_remove");
  EXPECT_EQ("hide_selector",
            content::EvalJs(
                web_contents(),
                "document.getElementById('hidden_then_remove').innerHTML = "
                "'hide_selector'"));
  VerifyTargetHidden(true, "hidden_then_remove");
  VerifyTargetRemoved(false, "hidden_then_remove");
  EXPECT_EQ("remove_selector",
            content::EvalJs(
                web_contents(),
                "document.getElementById('hidden_then_remove').innerHTML = "
                "'remove_selector'"));
  VerifyTargetRemoved(true, "hidden_then_remove");
  ASSERT_EQ(observer_.hidden_notifications_.size(), 1u);
  EXPECT_THAT(
      observer_.hidden_notifications_,
      testing::UnorderedElementsAre(
          ":root > BODY:nth-child(2) > DIV:nth-child(11) > SPAN:nth-child(4)"));
  ASSERT_EQ(observer_.removed_notifications_.size(), 1u);
  EXPECT_THAT(
      observer_.removed_notifications_,
      testing::UnorderedElementsAre(
          ":root > BODY:nth-child(2) > DIV:nth-child(11) > SPAN:nth-child(4)"));
}

IN_PROC_BROWSER_TEST_P(AdblockContentFiltersBrowserTest,
                       VerifyInlineCssStyleModificationLogic) {
  ASSERT_TRUE(
      content::NavigateToURL(shell(), GetUrl("/content_type_filters.html")));
  static constexpr char get_expected_style_property[] =
      "document.getElementById('hidden_then_inline_css').style['%s'] === '%s'";
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "background-color",
                         "rgb(255, 0, 0)")
          .c_str()));
  EXPECT_TRUE(WaitAndVerifyCondition(
      base::StringPrintf(get_expected_style_property, "width", "").c_str()));
  SetFilters(
      {"example.com###hidden_then_inline_css {background-color: "
       "#00FF00!important;}",
       "example.com###hidden_then_inline_css {width: 100px;}"});
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
  ASSERT_EQ(observer_.inline_css_notifications_.size(), 1u);
  EXPECT_THAT(observer_.inline_css_notifications_,
              testing::UnorderedElementsAre("#hidden_then_inline_css"));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockContentFiltersBrowserTest,
                         testing::Values(false /*normal*/, true /*incognito*/));

}  // namespace adblock
