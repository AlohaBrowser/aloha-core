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

#include "components/adblock/core/converter/parser/content_filter.h"

#include "base/logging.h"

namespace adblock {

static constexpr char kDomainSeparator[] = ",";

// static
absl::optional<ContentFilter> ContentFilter::FromString(
    std::string_view domain_list,
    FilterType filter_type,
    std::string_view selector) {
  DCHECK(filter_type == FilterType::ElemHide ||
         filter_type == FilterType::ElemHideException ||
         filter_type == FilterType::ElemHideEmulation ||
         filter_type == FilterType::Remove ||
         filter_type == FilterType::InlineCss);
  if (selector.empty()) {
    VLOG(1) << "[eyeo] Content filters require selector";
    return {};
  }

  DomainOption domains =
      DomainOption::FromString(domain_list, kDomainSeparator);

  if (filter_type == FilterType::ElemHideEmulation ||
      filter_type == FilterType::Remove ||
      filter_type == FilterType::InlineCss) {
    // ElemHideEmulation filters require that the domains have
    // at least one subdomain or is localhost
    domains.RemoveDomainsWithNoSubdomain();
    if (domains.GetIncludeDomains().empty()) {
      VLOG(1) << "[eyeo] ElemHideEmulation, Remove and InlineCss "
                 "filters require include domains.";
      return {};
    }
  } else if (selector.size() < 3 && domains.UnrestrictedByDomain()) {
    VLOG(1) << "[eyeo] Content filter is not specific enough.  Must be longer "
               "than 2 characters or restricted by domain.";
    return {};
  }

  std::string_view modifier = "";
  if (filter_type == FilterType::Remove ||
      filter_type == FilterType::InlineCss) {
    auto modifier_pos = selector.find_last_of('{');
    if (modifier_pos == std::string_view::npos) {
      VLOG(1) << "[eyeo] Mailformed modifier.";
      return {};
    }

    modifier = selector.substr(modifier_pos);

    selector.remove_suffix(modifier.length());
    selector = base::TrimWhitespaceASCII(selector, base::TRIM_TRAILING);
    if (selector.empty()) {
      VLOG(1) << "[eyeo] modifier requires a selector.";
      return {};
    }

    // Remove leading '{' and trailing '}'. base::TrimSTring(modifier, "{}",
    // base::TRIM_ALL) might invalidate the encolsed CSS.
    modifier.remove_prefix(1);
    modifier.remove_suffix(1);
    modifier = base::TrimWhitespaceASCII(modifier, base::TRIM_ALL);
  }

  return ContentFilter(filter_type, selector, modifier, std::move(domains));
}

ContentFilter::ContentFilter(FilterType type,
                             std::string_view selector,
                             std::string_view modifier,
                             DomainOption domains)
    : type(type),
      selector(selector.data(), selector.size()),
      modifier(modifier.data(), modifier.size()),
      domains(std::move(domains)) {}
ContentFilter::ContentFilter(const ContentFilter& other) = default;
ContentFilter::~ContentFilter() = default;

}  // namespace adblock
