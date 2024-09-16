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

#include "components/adblock/core/subscription/test/installed_subscription_impl_test_base.h"

#include "absl/types/variant.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockInstalledSubscriptionImplElemhideTest
    : public AdblockInstalledSubscriptionImplTestBase {};

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_generic_selector) {
  auto subscriptions = ConvertAndLoadRules("##.zad.billboard");
  auto selectors = subscriptions->GetElemhideData(
      GURL("https://pl.ign.com/marvels-avengers/41262/news/"
           "marvels-avengers-kratos-zagra-czarna-pantere"),
      false);
  EXPECT_EQ(FilterSelectors(selectors),
            std::set<std::string_view>({".zad.billboard"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest, Elementhide_excludes_sub) {
  auto subscriptions = ConvertAndLoadRules(R"(
        example.org###ad_1
        example.org###ad_2
        foo.example.org#@##ad_2
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideData(GURL("http://foo.example.org"), false);

  const auto selectors_2 =
      subscriptions->GetElemhideData(GURL("http://example.org"), false);

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<std::string_view>({"#ad_1"}));
  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<std::string_view>({"#ad_1", "#ad_2"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_domain_specific) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ! other type of filters
    /testcasefiles/blocking/addresspart/abptestcasepath/
    example.org#?#div:-abp-properties(width: 213px)
    ! element hiding selectors
    ###testcase-eh-id
    example.org###testcase-eh-id
    example.org##.testcase-eh-class
    example.org##.testcase-container > .testcase-eh-descendant
    ~foo.example.org,example.org##foo
    ~othersiteneg.org##testneg
    ! other site
    )");
  auto selectors =
      subscriptions->GetElemhideData(GURL("http://example.org"), false);
  EXPECT_EQ(std::set<std::string_view>(
                {"#testcase-eh-id", "testneg",
                 ".testcase-container > .testcase-eh-descendant",
                 ".testcase-eh-class", "foo"}),
            FilterSelectors(selectors));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest, Elementhide_same_result) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example1.org###testcase-eh-id
    example2.org###testcase-eh-id
    )");
  auto selectors_1 =
      subscriptions->GetElemhideData(GURL("http://example1.org"), false);

  auto selectors_2 =
      subscriptions->GetElemhideData(GURL("http://example2.org"), false);

  auto selectors_3 = subscriptions->GetElemhideData(
      GURL("http://non-existing-domain.com"), false);

  EXPECT_EQ(FilterSelectors(selectors_1), FilterSelectors(selectors_2));
  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<std::string_view>({"#testcase-eh-id"}));
  EXPECT_EQ(FilterSelectors(selectors_3).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_exception_main_domain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    sub.example.org###testcase-eh-id
    example.org#@##testcase-eh-id
    )");
  auto selectors =
      subscriptions->GetElemhideData(GURL("http://sub.example.org"), false);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_apply_just_domain) {
  auto subscriptions = ConvertAndLoadRules("example.org###div");

  auto selectors =
      subscriptions->GetElemhideData(GURL("http://example.org"), true);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>({"#div"}));
  auto selectors2 =
      subscriptions->GetElemhideData(GURL("http://example2.org"), false);

  EXPECT_EQ(FilterSelectors(selectors2).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest, Elementhideemu_generic) {
  auto subscriptions = ConvertAndLoadRules("example.org#?#foo");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>({"foo"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_allow_list) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_allow_list_2) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#?#another
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors),
            std::set<std::string_view>({"another"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_allow_list_3) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#?#another
    example2.org#?#foo
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("http://example2.org"));

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>({"foo"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_domain_n_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
      !other type of filters
      /testcasefiles/blocking/addresspart/abptestcasepath/
      example.org###testcase-eh-id
      !element hiding emulation selectors
      example.org#?#div:-abp-properties(width: 213px)
      example.org#?#div:-abp-has(>div>img.testcase-es-has)
      example.org#?#span:-abp-contains(ESContainsTarget)
      ~foo.example.org,example.org#?#div:-abp-properties(width: 213px)
      !allowlisted
      example.org#@#foo
      !other site
      othersite.com###testcase-eh-id
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideEmulationData(GURL("http://example.org"));

  // should be 3 unique selectors
  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<std::string_view>({
                "div:-abp-properties(width: 213px)",
                "div:-abp-has(>div>img.testcase-es-has)",
                "span:-abp-contains(ESContainsTarget)",
            }));

  const auto selectors_2 =
      subscriptions->GetElemhideEmulationData(GURL("http://foo.example.org"));

  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<std::string_view>({
                "div:-abp-properties(width: 213px)",
                "div:-abp-has(>div>img.testcase-es-has)",
                "span:-abp-contains(ESContainsTarget)",
            }));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_excludes_sub) {
  auto subscriptions = ConvertAndLoadRules(R"(
        example.org#?#general
        example.org#?#specific
        foo.example.org#@#specific
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideEmulationData(GURL("http://foo.example.org"));

  const auto selectors_2 =
      subscriptions->GetElemhideEmulationData(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<std::string_view>{"general"});

  EXPECT_EQ(FilterSelectors(selectors_2), std::set<std::string_view>({
                                              "general",
                                              "specific",
                                          }));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest, Elementhideemu_list_diff) {
  auto subscriptions = ConvertAndLoadRules(R"(
      example1.org#?#div:-abp-properties(width: 213px)
      example2.org#?#div:-abp-properties(width: 213px)
      example2.org#@#div:-abp-properties(width: 213px)
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideEmulationData(GURL("http://example1.org"));

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<std::string_view>{"div:-abp-properties(width: 213px)"});

  const auto selectors_2 =
      subscriptions->GetElemhideEmulationData(GURL("http://example2.org"));
  EXPECT_EQ(FilterSelectors(selectors_2).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_exception_with_excluded_url) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ###_AD
     ~imore.com#@##_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.imore.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>{"#_AD"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_with_generic_excluded_url) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ~imore.com###_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.domain.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>{"#_AD"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_top_tier_domain_match) {
  auto subscriptions = ConvertAndLoadRules(R"(
    com###_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.domain.com/"), true);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>{"#_AD"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_with_excluded_url_specific) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
     ~imore.com#@##_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.domain.com/"), true);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_with_excluded_url_case_insensitive) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
    ~IMore.com#@##_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.imore.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>{"#_AD"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_with_excluded_url_case_insensitive_2) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
    ~imore.com#@##_AD
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://www.IMORE.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<std::string_view>{"#_AD"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhide_exception_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ##.ad_box
    domain.com,~www.domain.com#@#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://domain.com/"), false);
  const auto selectors_2 =
      subscriptions->GetElemhideData(GURL("https://www.domain.com/"), false);
  const auto selectors_3 =
      subscriptions->GetElemhideData(GURL("https://domain2.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<std::string_view>{".ad_box"});
  EXPECT_EQ(FilterSelectors(selectors_3),
            std::set<std::string_view>{".ad_box"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_exception_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    domain.com#?#.ad_box
    mail.domain.com,~www.mail.domain.com#@#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("https://mail.domain.com/"));
  const auto selectors_2 = subscriptions->GetElemhideEmulationData(
      GURL("https://www.mail.domain.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);

  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<std::string_view>{".ad_box"});
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_without_include_domains) {
  // The Elemhide emu filter has no include domains, only an exclude domain,
  // which makes it generic. Elemhide emu filters cannot be generic, so we
  // don't apply this filter.
  auto subscriptions = ConvertAndLoadRules(R"(
    ~domain.com#?#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("https://test.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       Elementhideemu_on_top_level_domain) {
  // The Elemhide emu filter is defined to apply on all .com domains.
  // Elemhide emu filters cannot be applied so widely.
  auto subscriptions = ConvertAndLoadRules(R"(
    com#?#.ad_box
    .com#?#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationData(GURL("https://test.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest, EscapeSelectors) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com###foo[data-bar='{{foo: 1}}']
    )");
  const auto selectors =
      subscriptions->GetElemhideData(GURL("https://test.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors),
            std::set<std::string_view>(
                {"#foo[data-bar='\\7b \\7b foo: 1\\7d \\7d ']"}));
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       RemoveAcceptedFromPriviledgedList) {
  auto subscription = ConvertAndLoadRules(R"(
     example.com##foo {remove: true;}
     example.com#?#bar {remove: true;}
    )",
                                          {}, true);
  const auto eh_data =
      subscription->GetElemhideData(GURL("http://example.com"), false);
  const auto ehe_data =
      subscription->GetElemhideEmulationData(GURL("http://example.com"));
  EXPECT_EQ(0u, eh_data.remove_selectors.size());
  EXPECT_EQ(2u, ehe_data.remove_selectors.size());
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       RemoveAcceptedFromNonPriviledgedList) {
  auto subscription = ConvertAndLoadRules(R"(
     example.com##foo {remove: true;}
     example.com#?#bar {remove: true;}
    )",
                                          {}, false);
  const auto eh_data =
      subscription->GetElemhideData(GURL("http://example.com"), false);
  const auto ehe_data =
      subscription->GetElemhideEmulationData(GURL("http://example.com"));
  EXPECT_EQ(0u, eh_data.remove_selectors.size());
  EXPECT_EQ(2u, ehe_data.remove_selectors.size());
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       InlineCssAcceptedFromPriviledgedList) {
  auto subscription = ConvertAndLoadRules(R"(
     example.com##foo {color: #000;}
     example.com#?#bar {color: #000;}
    )",
                                          {}, true);
  const auto eh_data =
      subscription->GetElemhideData(GURL("http://example.com"), false);
  const auto ehe_data =
      subscription->GetElemhideEmulationData(GURL("http://example.com"));
  EXPECT_EQ(0u, eh_data.selectors_to_inline_css.size());
  EXPECT_EQ(2u, ehe_data.selectors_to_inline_css.size());
}

TEST_F(AdblockInstalledSubscriptionImplElemhideTest,
       InlineCssIgnoredFromNonPriviledgedList) {
  auto subscription = ConvertAndLoadRules(R"(
     example.com##foo {color: #000;}
     example.com#?#bar {color: #000;}
    )",
                                          {}, false);
  const auto eh_data =
      subscription->GetElemhideData(GURL("http://example.com"), false);
  const auto ehe_data =
      subscription->GetElemhideEmulationData(GURL("http://example.com"));
  EXPECT_EQ(0u, eh_data.selectors_to_inline_css.size());
  EXPECT_EQ(0u, ehe_data.selectors_to_inline_css.size());
}

enum class ContentFilterType {
  kElemhide,
  kElemhideEmulation,
};

class AdblockInstalledSubscriptionImplElemhideParametrizedTest
    : public ::testing::WithParamInterface<ContentFilterType>,
      public AdblockInstalledSubscriptionImplElemhideTest {
 protected:
  std::string separator() const {
    return GetParam() == ContentFilterType::kElemhide ? "##" : "#?#";
  }

  void ExpectSelectorsMatch(scoped_refptr<InstalledSubscription> subscription,
                            const std::string& url,
                            const std::set<std::string_view>& expected) {
    const auto selectors =
        GetParam() == ContentFilterType::kElemhide
            ? subscription->GetElemhideData(GURL(url), true)
            : subscription->GetElemhideEmulationData(GURL(url));
    EXPECT_EQ(FilterSelectors(selectors), expected);
  }
};

TEST_P(AdblockInstalledSubscriptionImplElemhideParametrizedTest,
       ElementhideMatchWildcardTld) {
  auto subscription = ConvertAndLoadRules("example.*" + separator() + "#div");
  const auto expected_selectors = std::set<std::string_view>({"#div"});
  // example.* matches any valid TLD.
  ExpectSelectorsMatch(subscription, "http://example.org", expected_selectors);
  ExpectSelectorsMatch(subscription, "http://example.com", expected_selectors);
  // Two-component TLD:
  ExpectSelectorsMatch(subscription, "http://example.com.br",
                       expected_selectors);
  // Subdomains:
  ExpectSelectorsMatch(subscription, "http://sub.example.com",
                       expected_selectors);
  ExpectSelectorsMatch(subscription, "http://sub.example.com.br",
                       expected_selectors);
}

TEST_P(AdblockInstalledSubscriptionImplElemhideParametrizedTest,
       ElementhideDoesNotMatchWildcardTld) {
  auto subscription = ConvertAndLoadRules("example.*" + separator() + "#div");
  const auto no_selectors = std::set<std::string_view>();
  // .evil is not a known TLD.
  ExpectSelectorsMatch(subscription, "http://example.evil", no_selectors);
  // .blogspot.com is a valid TLD but it's a private registrar.
  ExpectSelectorsMatch(subscription, "http://example.blogspot.com",
                       no_selectors);
  // notexample.com is a different domain than example.com, should not be
  // matched by example.*.
  ExpectSelectorsMatch(subscription, "http://notexample.com", no_selectors);
  // .abc.com is not a TLD at all.
  ExpectSelectorsMatch(subscription, "http://example.abc.com", no_selectors);
}

TEST_P(AdblockInstalledSubscriptionImplElemhideParametrizedTest,
       ElementhideMatchExcludedWildcardTld) {
  auto subscription = ConvertAndLoadRules("example.*,~subdomain.example.*" +
                                          separator() + "#div");
  const auto expected_selectors = std::set<std::string_view>({"#div"});
  const auto no_selectors = std::set<std::string_view>();
  // The filter should match example.org and example.com but not
  // subdomain.example.org or subdomain.example.com.
  ExpectSelectorsMatch(subscription, "http://example.com", expected_selectors);
  ExpectSelectorsMatch(subscription, "http://example.org", expected_selectors);
  ExpectSelectorsMatch(subscription, "http://subdomain.example.com",
                       no_selectors);
  ExpectSelectorsMatch(subscription, "http://subdomain.example.org",
                       no_selectors);
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockInstalledSubscriptionImplElemhideParametrizedTest,
    testing::Values(ContentFilterType::kElemhide,
                    ContentFilterType::kElemhideEmulation));

}  // namespace adblock
