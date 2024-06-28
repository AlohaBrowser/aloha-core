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

#include "components/adblock/core/classifier/resource_classifier_impl.h"

#include <string_view>

#include "absl/types/optional.h"
#include "base/memory/raw_ptr.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using testing::_;
using testing::Return;

using Decision = ResourceClassifier::ClassificationResult::Decision;

class AdblockResourceClassifierImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    classifier_ = base::MakeRefCounted<ResourceClassifierImpl>();
    mock_snapshot_ = std::make_unique<SubscriptionService::Snapshot>();
    mock_snapshot_->push_back(std::make_unique<MockSubscriptionCollection>());
    EXPECT_CALL(mock_subscription_collection(), GetFilteringConfigurationName())
        .WillRepeatedly(testing::ReturnRef(kConfigurationName));
  }

  ResourceClassifier::ClassificationResult ClassifyRequest() {
    return classifier_->ClassifyRequest(std::move(*mock_snapshot_),
                                        kResourceAddress, {kParentAddress},
                                        kContentType, kSitekey);
  }

  MockSubscriptionCollection& mock_subscription_collection() {
    return *static_cast<MockSubscriptionCollection*>(
        mock_snapshot_->front().get());
  }

  void FindBySubresourceFilterReturns(const absl::optional<GURL>& return_value,
                                      FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                FindBySubresourceFilter(_, _, _, _, category))
        .WillOnce(Return(return_value));
  }

  void FindBySpecialFilterReturns(const absl::optional<GURL>& return_value,
                                  SpecialFilterType type) {
    EXPECT_CALL(mock_subscription_collection(),
                FindBySpecialFilter(type, _, _, _))
        .WillOnce(Return(return_value));
  }

  void FindByAllowFilterReturns(const absl::optional<GURL>& return_value) {
    EXPECT_CALL(mock_subscription_collection(), FindByAllowFilter(_, _, _, _))
        .WillOnce(Return(return_value));
  }

  void FindByPopupFilterReturns(const absl::optional<GURL>& return_value,
                                FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                FindByPopupFilter(_, _, _, category))
        .WillOnce(Return(return_value));
  }

  void GetHeaderFiltersReturns(const std::set<HeaderFilterData>& return_value,
                               FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                GetHeaderFilters(_, _, _, category))
        .WillOnce(Return(return_value));
  }

  void AssertFindBySubresourceFilterNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                FindBySubresourceFilter(_, _, _, _, category))
        .Times(0);
  }

  void GetRewriteFiltersReturns(const std::set<std::string_view>& return_value,
                                FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                GetRewriteFilters(_, _, category))
        .WillOnce(Return(return_value));
  }

  void AssertFindBySpecialFilterNotCalled(SpecialFilterType type) {
    EXPECT_CALL(mock_subscription_collection(),
                FindBySpecialFilter(type, _, _, _))
        .Times(0);
  }

  void AssertFindByAllowFilterNotCalled() {
    EXPECT_CALL(mock_subscription_collection(), FindByAllowFilter(_, _, _, _))
        .Times(0);
  }

  void AssertFindByPopupFilterNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                FindByPopupFilter(_, _, _, category))
        .Times(0);
  }

  void AssertGetHeaderFiltersNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection(),
                GetHeaderFilters(_, _, _, category))
        .Times(0);
  }

  scoped_refptr<net::HttpResponseHeaders> CreateResponseHeaders(
      std::string_view header) {
    std::string full_headers = "HTTP/1.1 200 OK\n";
    full_headers.append(header.begin()).append(":");
    auto headers = net::HttpResponseHeaders::TryToCreate(full_headers);
    CHECK(headers);
    return headers;
  }
  const GURL kResourceAddress{"https://address.com/image.png"};
  const GURL kParentAddress{"https://parent.com/"};
  const ContentType kContentType = ContentType::Image;
  const SiteKey kSitekey{"abc"};
  const GURL kSourceUrl{"https://subscription.com/easylist.txt"};
  const std::set<HeaderFilterData> kAllowingHeaderFilters = {
      {"allowing_filter", kSourceUrl}};
  const std::set<HeaderFilterData> kBlockingHeaderFilters = {
      {"blocking_filter", kSourceUrl}};
  const std::set<HeaderFilterData> kDomainSpecificHeaderFilters = {
      {"domain_specific_filter", kSourceUrl}};
  const std::string kConfigurationName = "test_configuration";
  std::unique_ptr<SubscriptionService::Snapshot> mock_snapshot_;
  scoped_refptr<ResourceClassifier> classifier_;
};

TEST_F(AdblockResourceClassifierImplTest, BlockingFilterNotFound) {
  // Subscriptions get queried for url filters, no blocking filter is found.
  FindBySubresourceFilterReturns(absl::nullopt, FilterCategory::Blocking);

  // Since there's no blocking filter, all search stops now.
  AssertFindByAllowFilterNotCalled();
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest, BlockingFilterFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Subscriptions have no allowing filter, so the last chance of not
  // blocking the request is a generic block filter (separate test).
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, BlockingAndAllowingFilterFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::optional<GURL>(kSourceUrl));

  // Allowing filter has been found, make other searach stops now.
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Allowed);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, GenericBlockSearch) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found and genericblock is also not found.
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, DomainSpecificNotFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found but gebricblock exists.
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);

  // However there is no domain specific filter.
  FindBySubresourceFilterReturns(absl::nullopt,
                                 FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest, DomainSpecificFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found but gebricblock exists.
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);

  // And there is a domain specific filter.
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::DomainSpecificBlocking);

  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, PopupNoHasFilters) {
  FindByPopupFilterReturns(absl::nullopt, FilterCategory::Blocking);
  AssertFindByPopupFilterNotCalled(FilterCategory::Allowing);

  auto result = classifier_->ClassifyPopup(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress}, kSitekey);
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest, PopupHasBlockingFilter) {
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Blocking);
  FindByPopupFilterReturns(absl::nullopt, FilterCategory::Allowing);

  auto result = classifier_->ClassifyPopup(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress}, kSitekey);
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, PopupHasAllowingFilter) {
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Blocking);
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Allowing);

  auto result = classifier_->ClassifyPopup(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress}, kSitekey);
  EXPECT_EQ(result.decision, Decision::Allowed);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, NoBlockingHeaderFilters) {
  GetHeaderFiltersReturns({}, FilterCategory::Blocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, {});
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithDocumentAllowingFilter) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Document);

  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, {});
  EXPECT_EQ(result.decision, Decision::Allowed);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockButNoDomainSpecific) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns({}, FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, {});
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockAndDomainSpecificNoMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns(kDomainSpecificHeaderFilters,
                          FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto headers = CreateResponseHeaders("not_matching");
  CHECK(headers);
  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, headers);
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockAndDomainSpecificMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns(kDomainSpecificHeaderFilters,
                          FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto headers = CreateResponseHeaders(
      kDomainSpecificHeaderFilters.begin()->header_filter);
  CHECK(headers);
  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, headers);
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest, BlockingHeaderFiltersNoMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers = CreateResponseHeaders("no_match");
  CHECK(headers);
  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, headers);
  EXPECT_EQ(result.decision, Decision::Ignored);
  EXPECT_EQ(result.decisive_configuration_name, "");
  EXPECT_EQ(result.decisive_subscription, GURL());
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersMatchNoAllowingFilterMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers =
      CreateResponseHeaders(kBlockingHeaderFilters.begin()->header_filter);
  CHECK(headers);
  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, headers);
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersMatchWithAllowingFilterMatch) {
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers =
      CreateResponseHeaders(kAllowingHeaderFilters.begin()->header_filter);
  CHECK(headers);
  auto result = classifier_->ClassifyResponse(
      std::move(*mock_snapshot_), kResourceAddress, {kParentAddress},
      kContentType, headers);
  EXPECT_EQ(result.decision, Decision::Allowed);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest,
       TwoConfigs_DefaultAllowsOtherBlocks_BlockingFilterFound) {
  // Default configuration has allowing filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);
  FindByAllowFilterReturns(absl::optional<GURL>(kSourceUrl));

  // Other configuration has blocking filter
  std::string other_coniguration = "other";
  GURL other_source{"https://subscription.com/other.txt"};
  auto mock_subscription_collection =
      std::make_unique<MockSubscriptionCollection>();
  EXPECT_CALL(*mock_subscription_collection,
              FindBySubresourceFilter(_, _, _, _, FilterCategory::Blocking))
      .WillOnce(Return(absl::optional<GURL>(other_source)));
  EXPECT_CALL(*mock_subscription_collection, FindByAllowFilter(_, _, _, _))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*mock_subscription_collection,
              FindBySpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*mock_subscription_collection, GetFilteringConfigurationName())
      .WillRepeatedly(testing::ReturnRef(other_coniguration));

  mock_snapshot_->push_back(std::move(mock_subscription_collection));
  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, other_coniguration);
  EXPECT_EQ(result.decisive_subscription, other_source);
}

TEST_F(AdblockResourceClassifierImplTest,
       TwoConfigs_DefaultBlocksOtherIsNotChecked_BlockingFilterFound) {
  // Default configuration has blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);
  FindByAllowFilterReturns(absl::nullopt);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);

  // Other configuration should not be called
  auto mock_subscription_collection =
      std::make_unique<MockSubscriptionCollection>();
  EXPECT_CALL(*mock_subscription_collection,
              FindBySubresourceFilter(_, _, _, _, FilterCategory::Blocking))
      .Times(0);

  mock_snapshot_->push_back(std::move(mock_subscription_collection));
  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Blocked);
  EXPECT_EQ(result.decisive_configuration_name, kConfigurationName);
  EXPECT_EQ(result.decisive_subscription, kSourceUrl);
}

TEST_F(AdblockResourceClassifierImplTest,
       TwoConfigs_DefaultAndOtherAllows_AllowingFilterFound) {
  // Default configuration has allowing filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);
  FindByAllowFilterReturns(absl::optional<GURL>(kSourceUrl));

  // Other configuration has also allowing filter
  std::string other_coniguration = "other";
  GURL other_source{"https://subscription.com/other.txt"};
  MockSubscriptionCollection* mock_subscription_collection =
      new MockSubscriptionCollection();
  EXPECT_CALL(*mock_subscription_collection,
              FindBySubresourceFilter(_, _, _, _, FilterCategory::Blocking))
      .WillOnce(Return(other_source));
  EXPECT_CALL(*mock_subscription_collection, FindByAllowFilter(_, _, _, _))
      .WillOnce(Return(absl::optional<GURL>(other_source)));
  EXPECT_CALL(*mock_subscription_collection, GetFilteringConfigurationName())
      .WillRepeatedly(testing::ReturnRef(other_coniguration));

  mock_snapshot_->emplace_back(mock_subscription_collection);
  auto result = ClassifyRequest();
  EXPECT_EQ(result.decision, Decision::Allowed);
  EXPECT_EQ(result.decisive_configuration_name, other_coniguration);
  EXPECT_EQ(result.decisive_subscription, other_source);
}

TEST_F(AdblockResourceClassifierImplTest, RewriteFilterNotFound) {
  // There are no blocking filters found.
  GetRewriteFiltersReturns({}, FilterCategory::Blocking);

  // Since there are no blocking filters, no need to check allow filters.
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Document);
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);

  // Empty result means no redirect necessary.
  EXPECT_FALSE(classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                         kResourceAddress, {kParentAddress}));
}

TEST_F(AdblockResourceClassifierImplTest, RewriteFilterFound) {
  std::string_view redirect = "about::blank";

  GetRewriteFiltersReturns({redirect}, FilterCategory::Blocking);
  GetRewriteFiltersReturns({}, FilterCategory::Allowing);

  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);

  EXPECT_EQ(GURL{redirect},
            classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                      kResourceAddress, {kParentAddress}));
}

TEST_F(AdblockResourceClassifierImplTest, RewriteFilterDomainSpecificFound) {
  GetRewriteFiltersReturns({"about::blank/generic"}, FilterCategory::Blocking);
  GetRewriteFiltersReturns({}, FilterCategory::Allowing);

  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetRewriteFiltersReturns({"about::blank/domain_specific"},
                           FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(absl::optional<GURL>{GURL("about::blank/domain_specific")},
            classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                      kResourceAddress, {kParentAddress}));
}

TEST_F(AdblockResourceClassifierImplTest,
       RewriteBlockingFilterOverruledViaDocumentAllowingRule) {
  GetRewriteFiltersReturns({"about::blank"}, FilterCategory::Blocking);

  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Document);
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);

  // Empty result means no redirect necessary.
  EXPECT_FALSE(classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                         kResourceAddress, {kParentAddress}));
}

TEST_F(AdblockResourceClassifierImplTest,
       RewriteBlockingFilterOverruledViaOtherRewriteFilter) {
  GetRewriteFiltersReturns({"about::blank"}, FilterCategory::Blocking);

  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);

  GetRewriteFiltersReturns({"about::blank_not_matching", "about::blank"},
                           FilterCategory::Allowing);

  // Empty result means no redirect necessary.
  EXPECT_FALSE(classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                         kResourceAddress, {kParentAddress}));
}

TEST_F(AdblockResourceClassifierImplTest,
       TwoConfigs_RewriteFilterFound_OtherConfigNotChecked) {
  // Default configuration has blocking rewrite filter.
  std::string_view redirect = "about::blank";

  GetRewriteFiltersReturns({redirect}, FilterCategory::Blocking);
  GetRewriteFiltersReturns({}, FilterCategory::Allowing);

  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);

  // Other configuration should not be called
  auto mock_subscription_collection =
      std::make_unique<MockSubscriptionCollection>();
  EXPECT_CALL(*mock_subscription_collection, GetRewriteFilters(_, _, _))
      .Times(0);

  mock_snapshot_->push_back(std::move(mock_subscription_collection));
  EXPECT_EQ(GURL{redirect},
            classifier_->CheckRewrite(std::move(*mock_snapshot_),
                                      kResourceAddress, {kParentAddress}));
}

}  // namespace adblock
