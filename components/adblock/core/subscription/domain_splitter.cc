
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

#include "components/adblock/core/subscription/domain_splitter.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace adblock {

DomainSplitter::DomainSplitter(std::string_view domain)
    : remaining_domain_(base::TrimString(domain, ".", base::TRIM_ALL)) {
  // If the domain has a public registry, for "example info.domain.co.uk", we
  // prepare to also iterate over the domain without the registry, i.e.
  // "info.domain." in order to match wildcard filters.
  const size_t registry_length =
      net::registry_controlled_domains::GetCanonicalHostRegistryLength(
          domain,
          net::registry_controlled_domains::UnknownRegistryFilter::
              EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::PrivateRegistryFilter::
              EXCLUDE_PRIVATE_REGISTRIES);
  if (registry_length != 0) {
    domain_sans_registry_copy_ =
        domain.substr(0, domain.size() - registry_length);
    remaining_domain_sans_registry_ = domain_sans_registry_copy_;
  }
}

absl::optional<std::string_view> DomainSplitter::FindNextSubdomain() {
  std::string_view& remaining_domain = GetNonEmptyRemainingDomain();
  if (remaining_domain.empty()) {
    // We've exhausted both remaining_domain_ and
    // remaining_domain_sans_registry_, end iteration.
    return absl::nullopt;
  }

  const std::string_view return_value = remaining_domain;

  // Remove the next subdomain from remaining_domain:
  const size_t next_dot_index = remaining_domain.find('.');
  if (next_dot_index == std::string_view::npos) {
    remaining_domain = "";
  } else {
    remaining_domain.remove_prefix(next_dot_index + 1);
  }

  return return_value;
}

std::string_view& DomainSplitter::GetNonEmptyRemainingDomain() {
  if (!remaining_domain_.empty()) {
    return remaining_domain_;
  }
  return remaining_domain_sans_registry_;
}

}  // namespace adblock
