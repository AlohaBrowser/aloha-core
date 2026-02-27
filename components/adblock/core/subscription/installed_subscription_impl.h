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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_IMPL_H_

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "absl/types/optional.h"
#include "base/memory/raw_ptr.h"

#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/regex_matcher.h"

namespace adblock {

// A flatbuffer-based implementation of Subscription.
class InstalledSubscriptionImpl final : public InstalledSubscription {
 public:
  InstalledSubscriptionImpl(std::unique_ptr<FlatbufferData> buffer,
                            InstallationState installation_state,
                            base::Time installation_time);
  // Subscription
  GURL GetSourceUrl() const final;
  std::string GetTitle() const final;
  std::string GetCurrentVersion() const final;
  InstallationState GetInstallationState() const final;
  base::Time GetInstallationTime() const final;
  base::TimeDelta GetExpirationInterval() const final;

  // InstalledSubscription
  bool HasUrlFilter(const GURL& url,
                    const std::string& document_domain,
                    ContentType content_type,
                    const SiteKey& sitekey,
                    FilterCategory category) const final;
  bool HasPopupFilter(const GURL& url,
                      const std::string& document_domain,
                      const SiteKey& sitekey,
                      FilterCategory category) const final;
  bool HasSpecialFilter(SpecialFilterType type,
                        const GURL& url,
                        const std::string& document_domain,
                        const SiteKey& sitekey) const final;
  void FindCspFilters(const GURL& url,
                      const std::string& document_domain,
                      FilterCategory category,
                      std::set<std::string_view>& results) const final;
  std::set<std::string_view> FindRewriteFilters(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const final;
  void FindHeaderFilters(const GURL& url,
                         ContentType content_type,
                         const std::string& document_domain,
                         FilterCategory category,
                         std::set<HeaderFilterData>& results) const final;

  ContentFiltersData GetElemhideData(const GURL& url,
                                     bool domain_specific) const final;
  ContentFiltersData GetElemhideEmulationData(const GURL& url) const final;

  std::vector<Snippet> MatchSnippets(
      const std::string& document_domain) const final;

  void MarkForPermanentRemoval() final;

 private:
  friend class base::RefCountedThreadSafe<InstalledSubscriptionImpl>;
  ~InstalledSubscriptionImpl() final;
  enum class FindStrategy {
    FindFirst,
    FindAll,
  };

  using UrlFilterIndex =
      flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>;
  using Domains = flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>;
  // Finds the first filter in |category| that matches the remaining parameters.
  // Finds all filters in category that matchers the remaining parameters.
  std::vector<const flat::UrlFilter*> FindInternal(
      const UrlFilterIndex* index,
      const GURL& url,
      absl::optional<ContentType> content_type,
      const std::string& document_domain,
      const std::string& sitekey,
      FilterCategory category,
      FindStrategy strategy) const;
  void FindFiltersForKeyword(
      const UrlFilterIndex* index,
      std::string_view keyword,
      const GURL& url,
      const GURL& lowercase_url,
      absl::optional<ContentType> content_type,
      const std::string& document_domain,
      const std::string& sitekey,
      FilterCategory category,
      bool is_third_party_request,
      FindStrategy strategy,
      std::vector<const flat::UrlFilter*>& out_results) const;
  bool CandidateFilterViable(const flat::UrlFilter* candidate,
                             absl::optional<ContentType> content_type,
                             const std::string& document_domain,
                             const std::string& sitekey,
                             FilterCategory category,
                             bool is_third_party_request) const;
  bool IsGenericFilter(const flat::UrlFilter* filter) const;
  bool CheckThirdParty(const flat::UrlFilter* filter,
                       bool is_third_party_request) const;
  bool IsActiveOnDomain(const flat::UrlFilter* filter,
                        const std::string& document_domain,
                        const std::string& sitekey) const;
  bool IsActiveOnDomain(const std::string& document_domain,
                        const Domains* include_domains,
                        const Domains* exclude_domains) const;
  bool IsEmptyDomainAllowed(const Domains* include_domains,
                            const Domains* exclude_domains) const;

  const std::unique_ptr<FlatbufferData> buffer_;
  const InstallationState installation_state_;
  const base::Time installation_time_;
  raw_ptr<const flat::Subscription> index_ = nullptr;
  const std::unique_ptr<RegexMatcher> regex_matcher_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_IMPL_H_
