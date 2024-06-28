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

#include "components/adblock/core/subscription/subscription_collection_impl.h"

#include <string_view>
#include <vector>

#include "absl/types/optional.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/test/mock_installed_subscription.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using testing::_;
using testing::Return;

class AdblockSubscriptionCollectionImplTest : public ::testing::Test {
 public:
  const GURL kImageAddress{"https://address.com/image.png"};
  const GURL kParentAddress{"https://parent.com/"};
  const SiteKey kSitekey{"abc"};
  const GURL kSourceUrl{"https://subscription.com/easylist.txt"};
};

TEST_F(AdblockSubscriptionCollectionImplTest,
       AllowingFilterFoundForFrameHierarchy) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  const std::vector<GURL> frame_hierarchy{
      GURL("https://frame.com/frame1.html"),
      GURL("https://frame.com/frame2.html"),
      GURL("https://frame.com/"),
  };

  // Subscription has a blocking filter.
  EXPECT_CALL(*sub1, HasUrlFilter(kImageAddress, frame_hierarchy[0].host(),
                                  ContentType::Image, kSitekey,
                                  FilterCategory::Blocking))
      .WillOnce(Return(true));

  // The entire |frame_hierarchy| is queried to see if there's an allowing
  // filter present for any step in the chain. The final one reports a match.
  EXPECT_CALL(*sub1, HasUrlFilter(kImageAddress, frame_hierarchy[0].host(),
                                  ContentType::Image, kSitekey,
                                  FilterCategory::Allowing))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[0],
                               frame_hierarchy[1].host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[1],
                               frame_hierarchy[2].host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[2],
                               frame_hierarchy[2].host(), kSitekey))
      .WillOnce(Return(true));

  // The resource is allowlisted
  EXPECT_CALL(*sub1, GetSourceUrl()).WillRepeatedly(Return(kSourceUrl));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});
  EXPECT_TRUE(!!collection.FindBySubresourceFilter(
      kImageAddress, frame_hierarchy, ContentType::Image, kSitekey,
      FilterCategory::Blocking));
  EXPECT_TRUE(!!collection.FindByAllowFilter(kImageAddress, frame_hierarchy,
                                             ContentType::Image, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       ElemhideSelectorsCombinedFromSubscriptions) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First, we establish whether to search for domain-specific selectors only by
  // querying for Generichide filters. One subscription does have a Generichide
  // filter.

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Generichide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Generichide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));
  const bool domain_specific = true;

  // Now all subscriptions are queried for selectors.
  InstalledSubscription::ContentFiltersData sub1_selectors;
  sub1_selectors.elemhide_selectors = {"div", "ad_frame", "billboard"};
  sub1_selectors.elemhide_exceptions = {"billboard"};
  EXPECT_CALL(*sub1, GetElemhideData(kParentAddress, domain_specific))
      .WillOnce(Return(sub1_selectors));

  InstalledSubscription::ContentFiltersData sub2_selectors;
  sub2_selectors.elemhide_selectors = {"header", "ad_content", "billboard"};
  sub2_selectors.elemhide_exceptions = {"ad_frame"};
  EXPECT_CALL(*sub2, GetElemhideData(kParentAddress, domain_specific))
      .WillOnce(Return(sub2_selectors));

  // sub1's "billboard" exception cancels out the "billboard" selectors from
  // both subscriptions. sub2's "ad_frame" exception cancels out the "ad_frame"
  // selector from sub1.
  std::vector<std::string_view> expected_selectors{"div", "header",
                                                   "ad_content"};

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  auto actual_selectors =
      collection.GetElementHideData(kParentAddress, {}, kSitekey)
          .elemhide_selectors;
  std::sort(actual_selectors.begin(), actual_selectors.end());
  std::sort(expected_selectors.begin(), expected_selectors.end());
  EXPECT_EQ(actual_selectors, expected_selectors);
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       ElemhideEmulationSelectorsCombinedFromSubscriptions) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // This works very similarly to Elemhide selectors, only all Elemhide
  // Emulation selectors are by design domain-specific, so we don't search for
  // Generichide allow filters.

  // Now all subscriptions are queried for selectors.
  InstalledSubscription::ContentFiltersData sub1_selectors;
  sub1_selectors.elemhide_selectors = {"a", "b", "c"};
  sub1_selectors.elemhide_exceptions = {"c"};
  EXPECT_CALL(*sub1, GetElemhideEmulationData(kParentAddress))
      .WillOnce(Return(sub1_selectors));

  InstalledSubscription::ContentFiltersData sub2_selectors;
  sub2_selectors.elemhide_selectors = {"d", "b", "e"};
  sub2_selectors.elemhide_exceptions = {"b"};
  EXPECT_CALL(*sub2, GetElemhideEmulationData(kParentAddress))
      .WillOnce(Return(sub2_selectors));

  std::vector<std::string_view> expected_selectors{"a", "d", "e"};

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  auto actual_selectors =
      collection.GetElementHideEmulationData(kParentAddress).elemhide_selectors;
  std::sort(actual_selectors.begin(), actual_selectors.end());
  std::sort(expected_selectors.begin(), expected_selectors.end());
  EXPECT_EQ(actual_selectors, expected_selectors);
}

TEST_F(AdblockSubscriptionCollectionImplTest, GenerateSnippetsJson) {
  auto subscription = base::MakeRefCounted<MockInstalledSubscription>();

  InstalledSubscription::Snippet snippet;
  snippet.command = "say";
  snippet.arguments = {"Hello"};
  EXPECT_CALL(*subscription, MatchSnippets("parent.com"))
      .WillOnce(Return(std::vector<InstalledSubscription::Snippet>{snippet}));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{subscription}, {});
  auto snippets = collection.GenerateSnippets(kParentAddress, {});
  EXPECT_EQ("say", snippets.front().GetList().front().GetString());
  EXPECT_EQ("Hello", snippets.front().GetList().back().GetString());
  EXPECT_EQ("[ [ \"say\", \"Hello\" ] ]\n", snippets.DebugString());
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasAllowingDocumentFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_TRUE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Document, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, NoneHasAllowingDocumentFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_FALSE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Document, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasAllowingElementHideFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_TRUE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Elemhide, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       NoneHasAllowingElementHideFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_FALSE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Elemhide, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasGenericblockFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_TRUE(collection.FindBySpecialFilter(
      SpecialFilterType::Genericblock, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, NoneHasGenericblockFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_FALSE(collection.FindBySpecialFilter(
      SpecialFilterType::Genericblock, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, CspBlockingFilterNotFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // There are no blocking filters found.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _));

  // Since there are no blocking CSP filters, no need to check allow filters.
  EXPECT_CALL(*sub1, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub2, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub1, FindCspFilters(_, _, FilterCategory::Allowing, _))
      .Times(0);
  EXPECT_CALL(*sub2, FindCspFilters(_, _, FilterCategory::Allowing, _))
      .Times(0);

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});

  // Empty result means no CSP injection necessary.
  EXPECT_TRUE(
      collection.GetCspInjections(kImageAddress, {kParentAddress}).empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest, CspBlockingFilterFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("block"); })));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  // Check for Genericblock filter.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // In presence of a blocking CSP filters and absence of any allowing
  // filters, the string returned by first subscription becomes the CSP
  // injection.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});

  std::set<std::string_view> filters =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(filters, testing::UnorderedElementsAre("block"));
}

TEST_F(AdblockSubscriptionCollectionImplTest, MultipleCspBlockingFilterFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("first"); })));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("second"); })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  // Check for Genericblock filter.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // In presence of a blocking CSP filters and absence of any allowing
  // filters, the string returned by first subscription becomes the CSP
  // injection.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});

  std::set<std::string_view> filters =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(filters, testing::UnorderedElementsAre("first", "second"));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       SameCspBlockingFilterFoundInMultipleSubs) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("block"); })));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("block"); })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  // Check for Genericblock filter.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // In presence of a blocking CSP filters and absence of any allowing
  // filters, the string returned by first subscription becomes the CSP
  // injection.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});

  std::set<std::string_view> filters =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(filters, testing::UnorderedElementsAre("block"));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaDocumentAllowingRule) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src 'none'");
          })));

  // A document-wide allowing rule exists.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing Document filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});

  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaMatchingAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src 'none'");
          })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Document-wide allowing filters may or may not be consulted, but if they
  // are, there are no matches.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // An allowing CSP rule, with specific payload, is found for parent.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src 'none'");
          })));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       TwoCspBlockingFilterWithOneOverruledViaMatchingAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("first");
            res.insert("second");
          })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Document-wide allowing filters may or may not be consulted, but if they
  // are, there are no matches.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // An allowing CSP rule, with specific payload, is found for parent.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("second"); })));

  // Check for Genericblock filter.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(results, testing::UnorderedElementsAre("first"));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       TwoCspBlockingFilterOverruledViaMatchingAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("first"); })));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("second"); })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("second"); })));

  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("first"); })));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaGenericAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src 'none'");
          })));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Document-wide allowing filters may or may not be consulted, but if they
  // are, there are no matches.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // An allowing CSP rule, with no payload, is found for request. A CSP allowing
  // filter with no payload overrules any blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert(""); })));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterNotOverruledViaMismatchedAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src 'none'");
          })));

  // No document-wide allowing filters present.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Allowing CSP rules with different payloads do NOT match the blocking
  // CSP filter.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("script-src *");
          })));

  // The blocking filter is not overruled yet, will now check generic block
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // The blocking CSP filter was NOT overruled by an allowing CSP filter because
  // the payloads did not match.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});

  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(results, testing::UnorderedElementsAre("script-src 'none'"));
}

TEST_F(AdblockSubscriptionCollectionImplTest, GenericBlockCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter. Second does not.
  // Both are consulted, because the first subscription's blocking CSP filter
  // gets overruled as not domain-specific.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("block"); })));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _));

  // Neither subscription contains allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _));

  // Second subscription contains a genericblock rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // Search is retried but now for domain-specific CSP filters only. No matches.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::DomainSpecificBlocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("domain-block");
          })));

  EXPECT_CALL(*sub2, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::DomainSpecificBlocking, _));

  // Finally, no CSP filter found.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_THAT(results, testing::UnorderedElementsAre("domain-block"));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       GenericBlockCspFilterWithDomainAllowingFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter. Second does not.
  // Both are consulted, because the first subscription's blocking CSP filter
  // gets overruled as not domain-specific.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Blocking, _))
      .WillOnce(testing::WithArgs<3>(testing::Invoke(
          [](std::set<std::string_view>& res) { res.insert("block"); })));

  // Neither subscription contains allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::Allowing, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("domain-block");
          })));

  // Second subscription contains a genericblock rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // Search is retried but now for domain-specific CSP filters only. No matches.
  EXPECT_CALL(*sub1, FindCspFilters(kImageAddress, kParentAddress.host(),
                                    FilterCategory::DomainSpecificBlocking, _))
      .WillOnce(testing::WithArgs<3>(
          testing::Invoke([](std::set<std::string_view>& res) {
            res.insert("domain-block");
          })));

  // Finally, no CSP filter found.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1}, {});
  std::set<std::string_view> results =
      collection.GetCspInjections(kImageAddress, {kParentAddress});
  EXPECT_TRUE(results.empty());
}

TEST_F(AdblockSubscriptionCollectionImplTest, RewriteBlockingFilterNotFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // There are no blocking filters found.
  EXPECT_CALL(*sub1, FindRewriteFilters(kImageAddress, kParentAddress.host(),
                                        FilterCategory::Blocking))
      .WillOnce(Return(std::set<std::string_view>{}));
  EXPECT_CALL(*sub2, FindRewriteFilters(kImageAddress, kParentAddress.host(),
                                        FilterCategory::Blocking))
      .WillOnce(Return(std::set<std::string_view>{}));

  // Since there are no blocking filters, no need to check allow filters.
  EXPECT_CALL(*sub1, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub2, HasSpecialFilter(_, _, _, _)).Times(0);

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});

  // Empty result means no redirect necessary.
  EXPECT_THAT(collection.GetRewriteFilters(kImageAddress, {kParentAddress},
                                           FilterCategory::Blocking),
              testing::UnorderedElementsAre());
}

TEST_F(AdblockSubscriptionCollectionImplTest, RewriteBlockingFiltersFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();
  constexpr const char* redirect1 = "about::blank/1";
  constexpr const char* redirect2 = "about::blank/2";

  EXPECT_CALL(*sub1, FindRewriteFilters(kImageAddress, kParentAddress.host(),
                                        FilterCategory::Blocking))
      .WillOnce(Return(std::set<std::string_view>{redirect1}));
  EXPECT_CALL(*sub2, FindRewriteFilters(kImageAddress, kParentAddress.host(),
                                        FilterCategory::Blocking))
      .WillOnce(Return(std::set<std::string_view>{redirect2}));

  // In presence of a blocking filters and absence of any allowing filters,
  // the string returned by first subscription becomes the redirect.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2}, {});
  EXPECT_THAT(collection.GetRewriteFilters(kImageAddress, {kParentAddress},
                                           FilterCategory::Blocking),
              testing::UnorderedElementsAre(GURL(redirect1), GURL(redirect2)));
}

}  // namespace adblock
