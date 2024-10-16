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

#include <algorithm>
#include <numeric>
#include <string>

#include "base/ranges/algorithm.h"
#include "components/adblock/core/common/adblock_utils.h"

namespace adblock {
namespace {

std::string DocumentDomain(const GURL& request_url,
                           const std::vector<GURL>& frame_hierarchy) {
  return frame_hierarchy.empty() ? request_url.host()
                                 : frame_hierarchy[0].host();
}

InstalledSubscription::ContentFiltersData MaybeReduceSelectors(
    InstalledSubscription::ContentFiltersData& combined_selectors) {
  if (combined_selectors.elemhide_exceptions.empty()) {
    // Nothing to reduce.
    return combined_selectors;
  }
  // Populate result with blocking selectors.
  InstalledSubscription::ContentFiltersData final_selectors;
  final_selectors.elemhide_selectors =
      std::move(combined_selectors.elemhide_selectors);
  final_selectors.remove_selectors =
      std::move(combined_selectors.remove_selectors);
  final_selectors.selectors_to_inline_css =
      std::move(combined_selectors.selectors_to_inline_css);
  // Remove exceptions.
  for (auto* selectors_collection : {&final_selectors.elemhide_selectors,
                                     &final_selectors.remove_selectors}) {
    if (selectors_collection->empty()) {
      continue;
    }
    selectors_collection->erase(
        std::remove_if(
            selectors_collection->begin(), selectors_collection->end(),
            [&](const auto& selector) {
              return std::find(combined_selectors.elemhide_exceptions.begin(),
                               combined_selectors.elemhide_exceptions.end(),
                               selector) !=
                     combined_selectors.elemhide_exceptions.end();
            }),
        selectors_collection->end());
  }
  if (!final_selectors.selectors_to_inline_css.empty()) {
    final_selectors.selectors_to_inline_css.erase(
        std::remove_if(
            final_selectors.selectors_to_inline_css.begin(),
            final_selectors.selectors_to_inline_css.end(),
            [&](const auto& selector_with_css) {
              return std::find(combined_selectors.elemhide_exceptions.begin(),
                               combined_selectors.elemhide_exceptions.end(),
                               selector_with_css.first) !=
                     combined_selectors.elemhide_exceptions.end();
            }),
        final_selectors.selectors_to_inline_css.end());
  }
  return final_selectors;
}

bool SubscriptionContainsSpecialFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    SpecialFilterType filter_type,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) {
  for (auto it = frame_hierarchy.begin(); it < frame_hierarchy.end(); ++it) {
    const GURL& current_url = *it;
    const std::string& current_domain = std::next(it) != frame_hierarchy.end()
                                            ? std::next(it)->host()
                                            : current_url.host();
    if (subscription->HasSpecialFilter(filter_type, current_url, current_domain,
                                       sitekey)) {
      return true;
    }
  }
  return false;
}

bool HasAllowFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) {
  if (subscription->HasUrlFilter(
          request_url, DocumentDomain(request_url, frame_hierarchy),
          content_type, sitekey, FilterCategory::Allowing)) {
    return true;
  }
  if (SubscriptionContainsSpecialFilter(subscription,
                                        SpecialFilterType::Document,
                                        frame_hierarchy, sitekey)) {
    return true;
  }
  return false;
}

bool HasSpecialFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    SpecialFilterType filter_type,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) {
  if (subscription->HasSpecialFilter(
          filter_type, request_url,
          DocumentDomain(request_url, frame_hierarchy), sitekey)) {
    return true;
  }
  return SubscriptionContainsSpecialFilter(subscription, filter_type,
                                           frame_hierarchy, sitekey);
}

}  // namespace

SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    std::vector<scoped_refptr<InstalledSubscription>> current_state,
    const std::string& configuration_name)
    : subscriptions_(std::move(current_state)),
      configuration_name_(configuration_name) {}

SubscriptionCollectionImpl::~SubscriptionCollectionImpl() = default;
SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    const SubscriptionCollectionImpl&) = default;
SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    SubscriptionCollectionImpl&&) = default;
SubscriptionCollectionImpl& SubscriptionCollectionImpl::operator=(
    const SubscriptionCollectionImpl&) = default;
SubscriptionCollectionImpl& SubscriptionCollectionImpl::operator=(
    SubscriptionCollectionImpl&&) = default;

const std::string& SubscriptionCollectionImpl::GetFilteringConfigurationName()
    const {
  return configuration_name_;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindBySubresourceFilter(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey,
    FilterCategory category) const {
  const auto subscription = std::find_if(
      subscriptions_.begin(), subscriptions_.end(),
      [&](const auto& subscription) {
        return subscription->HasUrlFilter(
            request_url, DocumentDomain(request_url, frame_hierarchy),
            content_type, sitekey, category);
      });
  if (subscription != subscriptions_.end()) {
    return (*subscription)->GetSourceUrl();
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindByPopupFilter(
    const GURL& popup_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey,
    FilterCategory category) const {
  const auto subscription =
      std::find_if(subscriptions_.begin(), subscriptions_.end(),
                   [&](const auto& subscription) {
                     return subscription->HasPopupFilter(
                         popup_url, DocumentDomain(popup_url, frame_hierarchy),
                         sitekey, category);
                   });
  if (subscription != subscriptions_.end()) {
    return (*subscription)->GetSourceUrl();
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindByAllowFilter(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) const {
  for (const auto& subscription : subscriptions_) {
    if (HasAllowFilter(subscription, request_url, frame_hierarchy, content_type,
                       sitekey)) {
      return (*subscription).GetSourceUrl();
    }
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindBySpecialFilter(
    SpecialFilterType filter_type,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) const {
  for (const auto& subscription : subscriptions_) {
    if (HasSpecialFilter(subscription, filter_type, request_url,
                         frame_hierarchy, sitekey)) {
      return (*subscription).GetSourceUrl();
    }
  }
  return absl::nullopt;
}

InstalledSubscription::ContentFiltersData
SubscriptionCollectionImpl::GetElementHideData(
    const GURL& frame_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) const {
  const bool domain_specific = !!FindBySpecialFilter(
      SpecialFilterType::Generichide, frame_url, frame_hierarchy, sitekey);

  InstalledSubscription::ContentFiltersData combined_selectors;
  for (const auto& subscription : subscriptions_) {
    auto selectors = subscription->GetElemhideData(frame_url, domain_specific);
    std::move(selectors.elemhide_exceptions.begin(),
              selectors.elemhide_exceptions.end(),
              std::back_inserter(combined_selectors.elemhide_exceptions));
    std::move(selectors.elemhide_selectors.begin(),
              selectors.elemhide_selectors.end(),
              std::back_inserter(combined_selectors.elemhide_selectors));
  }
  return MaybeReduceSelectors(combined_selectors);
}

InstalledSubscription::ContentFiltersData
SubscriptionCollectionImpl::GetElementHideEmulationData(
    const GURL& frame_url) const {
  InstalledSubscription::ContentFiltersData combined_selectors;
  for (const auto& subscription : subscriptions_) {
    auto selectors = subscription->GetElemhideEmulationData(frame_url);
    std::move(selectors.elemhide_exceptions.begin(),
              selectors.elemhide_exceptions.end(),
              std::back_inserter(combined_selectors.elemhide_exceptions));
    std::move(selectors.elemhide_selectors.begin(),
              selectors.elemhide_selectors.end(),
              std::back_inserter(combined_selectors.elemhide_selectors));
    std::move(selectors.remove_selectors.begin(),
              selectors.remove_selectors.end(),
              std::back_inserter(combined_selectors.remove_selectors));
    std::move(selectors.selectors_to_inline_css.begin(),
              selectors.selectors_to_inline_css.end(),
              std::back_inserter(combined_selectors.selectors_to_inline_css));
  }
  return MaybeReduceSelectors(combined_selectors);
}

base::Value::List SubscriptionCollectionImpl::GenerateSnippets(
    const GURL& frame_url,
    const std::vector<GURL>& frame_hierarchy) const {
  base::Value::List snippets;
  auto document_domain = DocumentDomain(frame_url, frame_hierarchy);

  for (const auto& subscription : subscriptions_) {
    auto matched = subscription->MatchSnippets(document_domain);
    for (const auto& snippet : matched) {
      base::Value::List call;
      call.Append(base::Value(snippet.command));
      for (const auto& arg : snippet.arguments) {
        call.Append(base::Value(arg));
      }
      snippets.Append(std::move(call));
    }
  }

  return snippets;
}

std::set<std::string_view> SubscriptionCollectionImpl::GetCspInjections(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) const {
  std::set<std::string_view> blocking_filters{};
  std::set<std::string_view> allowing_filters{};
  for (const auto& subscription : subscriptions_) {
    subscription->FindCspFilters(request_url,
                                 DocumentDomain(request_url, frame_hierarchy),
                                 FilterCategory::Blocking, blocking_filters);
  }
  if (blocking_filters.empty()) {
    return {};
  }

  // If blocking filters found, check if can be overruled by allowing filters.
  for (const auto& subscription : subscriptions_) {
    // There may exist an allowing rule for this request and its immediate
    // parent frame. We also check for document-wide allowing filters.
    if (HasSpecialFilter(subscription, SpecialFilterType::Document, request_url,
                         frame_hierarchy, SiteKey())) {
      return {};
    }
    subscription->FindCspFilters(request_url,
                                 DocumentDomain(request_url, frame_hierarchy),
                                 FilterCategory::Allowing, allowing_filters);
  }

  // Remove overruled filters.
  for (const auto& a_f : allowing_filters) {
    if (a_f.empty()) {
      return {};
    }
    blocking_filters.erase(a_f);
  }
  if (blocking_filters.empty()) {
    return {};
  }

  // Last chance to avoid blocking: maybe there is a Genericblock filter and
  // we should re-search for domain-specific filters only?
  if (base::ranges::any_of(subscriptions_, [&](const auto& sub) {
        return HasSpecialFilter(sub, SpecialFilterType::Genericblock,
                                request_url, frame_hierarchy, SiteKey());
      })) {
    // This is a relatively rare case - we should have searched for
    // domain-specific filters only.
    std::set<std::string_view> domain_specific_blocking{};
    for (const auto& subscription : subscriptions_) {
      subscription->FindCspFilters(
          request_url, DocumentDomain(request_url, frame_hierarchy),
          FilterCategory::DomainSpecificBlocking, domain_specific_blocking);
      // There is a domain-specific blocking filter. No point in
      // searching for a domain-specific allowing filter, since the
      // previous search for non-specific allowing filters would have found
      // it.
    }
    if (!domain_specific_blocking.empty()) {
      for (const auto& a_f : allowing_filters) {
        if (a_f.empty()) {
          return {};
        }
        domain_specific_blocking.erase(a_f);
      }
    }

    return domain_specific_blocking;
  }

  return blocking_filters;
}

std::set<std::string_view> SubscriptionCollectionImpl::GetRewriteFilters(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    FilterCategory category) const {
  std::set<std::string_view> result;
  for (const auto& subscription : subscriptions_) {
    const auto filters = subscription->FindRewriteFilters(
        request_url, DocumentDomain(request_url, frame_hierarchy), category);
    result.insert(filters.begin(), filters.end());
  }
  return result;
}

std::set<HeaderFilterData> SubscriptionCollectionImpl::GetHeaderFilters(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    FilterCategory category) const {
  std::set<HeaderFilterData> filters{};
  for (const auto& subscription : subscriptions_) {
    subscription->FindHeaderFilters(
        request_url, content_type, DocumentDomain(request_url, frame_hierarchy),
        category, filters);
  }
  return filters;
}

}  // namespace adblock
