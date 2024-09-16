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

#include "components/adblock/core/converter/parser/domain_option.h"
#include "components/adblock/core/converter/parser/content_filter.h"

#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace adblock {

namespace {

void RemoveDuplicates(std::vector<std::string>& data) {
  sort(data.begin(), data.end());
  auto unique_end = unique(data.begin(), data.end());
  data.erase(unique_end, data.end());
}

}  // namespace

DomainOption::DomainOption() {}

// static
DomainOption DomainOption::FromString(std::string_view domains_list,
                                      std::string_view separator) {
  static const std::string_view kExclusionPrefix = "~";
  auto lower_domains_list = base::ToLowerASCII(domains_list);
  auto domains =
      base::SplitStringPiece(lower_domains_list, separator,
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Remove any trailing dots (e.g. "example.com." -> "example.com").
  for (auto& domain : domains) {
    domain = base::TrimString(domain, ".", base::TRIM_TRAILING);
  }

  // Remove trailing wildcard (e.g. "example.*" -> "example."). This works in
  // concert with DomainSplitter to match wildcard TLDs.
  // Domains that ends with a dot ("example.") should match any TLD
  // ("example.com", "example.co.uk", etc.).
  for (auto& domain : domains) {
    domain = base::TrimString(domain, "*", base::TRIM_TRAILING);
  }

  const auto first_include_domain_it = std::partition(
      domains.begin(), domains.end(), [](std::string_view domain) {
        return base::StartsWith(domain, kExclusionPrefix);
      });

  std::vector<std::string> exclude_domains(domains.begin(),
                                           first_include_domain_it);
  std::vector<std::string> include_domains(first_include_domain_it,
                                           domains.end());

  // Remove the ~ prefix that indicates an exclude domain.
  for (auto& domain : exclude_domains) {
    base::RemoveChars(domain, kExclusionPrefix, &domain);
  }

  RemoveDuplicates(exclude_domains);
  RemoveDuplicates(include_domains);

  return DomainOption(std::move(exclude_domains), std::move(include_domains));
}

const std::vector<std::string>& DomainOption::GetExcludeDomains() const {
  return exclude_domains_;
}

const std::vector<std::string>& DomainOption::GetIncludeDomains() const {
  return include_domains_;
}

void DomainOption::RemoveDomainsWithNoSubdomain() {
  exclude_domains_.erase(
      base::ranges::remove_if(
          exclude_domains_,
          [](auto it) { return !HasSubdomainOrLocalhost(it); }),
      exclude_domains_.end());

  include_domains_.erase(
      base::ranges::remove_if(
          include_domains_,
          [](auto it) { return !HasSubdomainOrLocalhost(it); }),
      include_domains_.end());
}

bool DomainOption::UnrestrictedByDomain() const {
  return base::ranges::count_if(exclude_domains_, &HasSubdomainOrLocalhost) ==
             0 &&
         base::ranges::count_if(include_domains_, &HasSubdomainOrLocalhost) ==
             0;
}

DomainOption::DomainOption(std::vector<std::string> exclude_domains,
                           std::vector<std::string> include_domains)
    : exclude_domains_(std::move(exclude_domains)),
      include_domains_(std::move(include_domains)) {}
DomainOption::DomainOption(const DomainOption& other) = default;
DomainOption::DomainOption(DomainOption&& other) = default;
DomainOption& DomainOption::operator=(const DomainOption& other) = default;
DomainOption& DomainOption::operator=(DomainOption&& other) = default;
DomainOption::~DomainOption() = default;

// static
bool DomainOption::HasSubdomainOrLocalhost(std::string_view domain) {
  return (domain == "localhost") ||
         (domain.find(".") != std::string_view::npos);
}

}  // namespace adblock
