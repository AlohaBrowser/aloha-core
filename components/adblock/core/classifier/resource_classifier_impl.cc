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

#include "base/strings/string_split.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/subscription/installed_subscription.h"

namespace adblock {
namespace {

using ClassificationResult = ResourceClassifier::ClassificationResult;

absl::optional<HeaderFilterData> IsHeaderFilterOverruled(
    std::string_view blocking_header_filter,
    std::set<HeaderFilterData>& allowing_filters) {
  for (auto filter : allowing_filters) {
    if (filter.header_filter.empty()) {
      // Allowing header filters may not contain payload, allow all headers in
      // that case.
      return filter;
    }
    if (utils::RegexMatches(filter.header_filter, blocking_header_filter,
                            true)) {
      return filter;
    }
  }
  return absl::nullopt;
}

bool ContainsHeaderValue(const scoped_refptr<net::HttpResponseHeaders>& headers,
                         std::string_view header_name,
                         const std::string& header_value) {
  size_t iter = 0;
  std::string value;
  while (headers->EnumerateHeader(&iter, header_name, &value)) {
    if (value.find(header_value) != std::string::npos) {
      return true;
    }
  }
  return false;
}

ClassificationResult ClassifyRequestWithSingleCollection(
    const SubscriptionCollection& subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) {
  // Search all subscriptions for any blocking filters (generic or
  // domain-specific).
  const auto subscription_with_blocking_filter_it =
      subscription_collection.FindBySubresourceFilter(
          request_url, frame_hierarchy, content_type, sitekey,
          FilterCategory::Blocking);
  if (!subscription_with_blocking_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{
        ClassificationResult::Decision::Ignored, {}, {}};
  }
  // Found a blocking filter but perhaps one of the subscriptions has an
  // allowing filter to override it?
  const auto subscription_with_allowing_filter_it =
      subscription_collection.FindByAllowFilter(request_url, frame_hierarchy,
                                                content_type, sitekey);
  if (subscription_with_allowing_filter_it) {
    // Found an overriding allowing filter:
    return ClassificationResult{
        ClassificationResult::Decision::Allowed,
        *subscription_with_allowing_filter_it,
        subscription_collection.GetFilteringConfigurationName()};
  }
  // Last chance to avoid blocking: maybe there is a GENERICBLOCK filter and we
  // should re-search for domain-specific filters only?
  if (subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Genericblock, request_url, frame_hierarchy,
          sitekey)) {
    // This is a relatively rare case - we should have searched for
    // domain-specific filters only.
    const auto subscription_with_domain_specific_blocking_filter_it =
        subscription_collection.FindBySubresourceFilter(
            request_url, frame_hierarchy, content_type, sitekey,
            FilterCategory::DomainSpecificBlocking);
    if (subscription_with_domain_specific_blocking_filter_it) {
      // There was a domain-specific blocking filter, the resource is blocked by
      // it.
      return ClassificationResult{
          ClassificationResult::Decision::Blocked,
          *subscription_with_domain_specific_blocking_filter_it,
          subscription_collection.GetFilteringConfigurationName()};
    } else {
      // There were no domain-specific blocking filters, our first match must
      // have been a generic filter.
      return ClassificationResult{
          ClassificationResult::Decision::Ignored, {}, {}};
    }
  }
  // There was no GENERICBLOCK filter available, so the original blocking result
  // is valid.
  return ClassificationResult{
      ClassificationResult::Decision::Blocked,
      *subscription_with_blocking_filter_it,
      subscription_collection.GetFilteringConfigurationName()};
}

ClassificationResult ClassifyPopupWithSingleCollection(
    const SubscriptionCollection& subscription_collection,
    const GURL& popup_url,
    const std::vector<GURL>& opener_frame_hierarchy,
    const SiteKey& sitekey) {
  // Search all subscriptions for popup blocking filters (generic or
  // domain-specific).
  const auto subscription_with_blocking_filter_it =
      subscription_collection.FindByPopupFilter(
          popup_url, opener_frame_hierarchy, sitekey, FilterCategory::Blocking);
  if (!subscription_with_blocking_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{
        ClassificationResult::Decision::Ignored, {}, {}};
  }
  // Found a blocking filter but perhaps one of the subscriptions has an
  // allowing filter to override it?
  const auto subscription_with_allowing_filter_it =
      subscription_collection.FindByPopupFilter(
          popup_url, opener_frame_hierarchy, sitekey, FilterCategory::Allowing);
  if (subscription_with_allowing_filter_it) {
    // Found an overriding allowing filter:
    return ClassificationResult{
        ClassificationResult::Decision::Allowed,
        *subscription_with_allowing_filter_it,
        subscription_collection.GetFilteringConfigurationName()};
  }
  const auto subscription_with_document_allowing_filter_it =
      subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Document, popup_url, opener_frame_hierarchy,
          sitekey);
  if (subscription_with_document_allowing_filter_it) {
    // Found an overriding document allowing filter for the frame hierarchy:
    return ClassificationResult{
        ClassificationResult::Decision::Allowed,
        *subscription_with_document_allowing_filter_it,
        subscription_collection.GetFilteringConfigurationName()};
  }
  // There is no overriding allowing filter, the popup should be blocked.
  return ClassificationResult{
      ClassificationResult::Decision::Blocked,
      *subscription_with_blocking_filter_it,
      subscription_collection.GetFilteringConfigurationName()};
}

ClassificationResult CheckHeaderFiltersMatchResponseHeaders(
    const SubscriptionCollection& subscription_collection,
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    std::set<HeaderFilterData> blocking_filters,
    std::set<HeaderFilterData> allowing_filters) {
  ClassificationResult result{ClassificationResult::Decision::Ignored, {}, {}};

  for (const auto& filter : blocking_filters) {
    auto key_value =
        base::SplitString(filter.header_filter, "=", base::KEEP_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);
    // If no '=' occurs, filter blocks response contains header, regardless
    // header value
    if (key_value.size() == 1u) {
      if (headers->HasHeader(filter.header_filter)) {
        if (auto allow_rule = IsHeaderFilterOverruled(filter.header_filter,
                                                      allowing_filters)) {
          result = {ClassificationResult::Decision::Allowed,
                    allow_rule->subscription_url,
                    subscription_collection.GetFilteringConfigurationName()};
        } else {
          return ClassificationResult{
              ClassificationResult::Decision::Blocked, filter.subscription_url,
              subscription_collection.GetFilteringConfigurationName()};
        }
      }
    } else {
      DCHECK_EQ(2u, key_value.size());
      if (ContainsHeaderValue(headers, key_value[0], key_value[1])) {
        if (auto allow_rule = IsHeaderFilterOverruled(filter.header_filter,
                                                      allowing_filters)) {
          result = {ClassificationResult::Decision::Allowed,
                    allow_rule->subscription_url,
                    subscription_collection.GetFilteringConfigurationName()};
        } else {
          return ClassificationResult{
              ClassificationResult::Decision::Blocked, filter.subscription_url,
              subscription_collection.GetFilteringConfigurationName()};
        }
      }
    }
  }
  return result;
}

ClassificationResult ClassifyResponseWithSingleCollection(
    const SubscriptionCollection& subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const scoped_refptr<net::HttpResponseHeaders>& response_headers) {
  auto blocking_filters = subscription_collection.GetHeaderFilters(
      request_url, frame_hierarchy, content_type, FilterCategory::Blocking);
  if (blocking_filters.empty()) {
    return ClassificationResult{
        ClassificationResult::Decision::Ignored, {}, {}};
  }

  // If blocking filters found, check first if filters are not overruled
  const auto subscription_with_allowing_document_filter_it =
      subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Document, request_url, frame_hierarchy, SiteKey());
  if (subscription_with_allowing_document_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{
        ClassificationResult::Decision::Allowed,
        *subscription_with_allowing_document_filter_it,
        subscription_collection.GetFilteringConfigurationName()};
  }

  if (subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Genericblock, request_url, frame_hierarchy,
          SiteKey())) {
    // If genericblock filter found, searched for blocking domain-specific
    // filters.
    blocking_filters = subscription_collection.GetHeaderFilters(
        request_url, frame_hierarchy, content_type,
        FilterCategory::DomainSpecificBlocking);

    return CheckHeaderFiltersMatchResponseHeaders(
        subscription_collection, request_url, frame_hierarchy, response_headers,
        std::move(blocking_filters), {});
  }
  // If no special filters found, get allowing filters and check which filters
  // applies.
  auto allowing_filters = subscription_collection.GetHeaderFilters(
      request_url, frame_hierarchy, content_type, FilterCategory::Allowing);
  return CheckHeaderFiltersMatchResponseHeaders(
      subscription_collection, request_url, frame_hierarchy, response_headers,
      std::move(blocking_filters), std::move(allowing_filters));
}

absl::optional<GURL> CheckRewriteWithSingleCollection(
    const SubscriptionCollection& subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) {
  auto blocking_rewrites = subscription_collection.GetRewriteFilters(
      request_url, frame_hierarchy, FilterCategory::Blocking);
  if (blocking_rewrites.empty()) {
    return absl::nullopt;
  }

  // If blocking filters are found, check first if blocking filters are not
  // overruled completely.
  const auto subscription_with_allowing_document_filter_it =
      subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Document, request_url, frame_hierarchy, SiteKey());
  if (subscription_with_allowing_document_filter_it) {
    return absl::nullopt;
  }

  if (subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Genericblock, request_url, frame_hierarchy,
          SiteKey())) {
    // If genericblock filter is found, searched for blocking domain-specific
    // filters.
    blocking_rewrites = subscription_collection.GetRewriteFilters(
        request_url, frame_hierarchy, FilterCategory::DomainSpecificBlocking);

    if (blocking_rewrites.empty()) {
      return absl::nullopt;
    }
  }

  // Check if blocking filters are not overruled by allowing ones.
  auto allowing_rewrites = subscription_collection.GetRewriteFilters(
      request_url, frame_hierarchy, FilterCategory::Allowing);
  if (allowing_rewrites.empty()) {
    // Any filter will do, take the 1st one.
    return absl::optional<GURL>(GURL{*blocking_rewrites.begin()});
  }

  // Change set to vector to call algorithm
  std::vector<std::string> blocking_rewrites_v(blocking_rewrites.begin(),
                                               blocking_rewrites.end());
  // Remove blocking filters overruled by allowing filters.
  blocking_rewrites_v.erase(
      base::ranges::remove_if(
          blocking_rewrites_v,
          [&allowing_rewrites](const auto blocking_rewrite) {
            return base::ranges::find_if(
                       allowing_rewrites, [&](const auto& allowing_rewrite) {
                         return blocking_rewrite == allowing_rewrite;
                       }) != allowing_rewrites.end();
          }),
      blocking_rewrites_v.end());

  if (blocking_rewrites_v.empty()) {
    return absl::nullopt;
  }

  // Any filter will do, take the 1st one.
  return absl::optional<GURL>(GURL{*blocking_rewrites_v.begin()});
}

}  // namespace

ResourceClassifierImpl::~ResourceClassifierImpl() = default;

ClassificationResult ResourceClassifierImpl::ClassifyRequest(
    const SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) const {
  auto classification =
      ClassificationResult{ClassificationResult::Decision::Ignored, {}, {}};
  for (const auto& collection : subscription_collections) {
    auto result = ClassifyRequestWithSingleCollection(
        *collection, request_url, frame_hierarchy, content_type, sitekey);
    if (result.decision == ClassificationResult::Decision::Blocked) {
      return result;
    }
    if (result.decision == ClassificationResult::Decision::Allowed) {
      classification = result;
    }
  }
  return classification;
}

ClassificationResult ResourceClassifierImpl::ClassifyPopup(
    const SubscriptionService::Snapshot& subscription_collections,
    const GURL& popup_url,
    const std::vector<GURL>& opener_frame_hierarchy,
    const SiteKey& sitekey) const {
  auto classification =
      ClassificationResult{ClassificationResult::Decision::Ignored, {}, {}};
  for (const auto& collection : subscription_collections) {
    auto result = ClassifyPopupWithSingleCollection(
        *collection, popup_url, opener_frame_hierarchy, sitekey);
    if (result.decision == ClassificationResult::Decision::Blocked) {
      return result;
    }
    if (result.decision == ClassificationResult::Decision::Allowed) {
      classification = result;
    }
  }
  return classification;
}

ClassificationResult ResourceClassifierImpl::ClassifyResponse(
    const SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const scoped_refptr<net::HttpResponseHeaders>& response_headers) const {
  auto classification =
      ClassificationResult{ClassificationResult::Decision::Ignored, {}, {}};
  for (const auto& collection : subscription_collections) {
    auto result = ClassifyResponseWithSingleCollection(
        *collection, request_url, frame_hierarchy, content_type,
        response_headers);
    if (result.decision == ClassificationResult::Decision::Blocked) {
      return result;
    }
    if (result.decision == ClassificationResult::Decision::Allowed) {
      classification = result;
    }
  }
  return classification;
}

absl::optional<GURL> ResourceClassifierImpl::CheckRewrite(
    const SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) const {
  for (const auto& collection : subscription_collections) {
    auto result = CheckRewriteWithSingleCollection(*collection, request_url,
                                                   frame_hierarchy);
    if (result) {
      return result;
    }
  }
  return absl::nullopt;
}

}  // namespace adblock
