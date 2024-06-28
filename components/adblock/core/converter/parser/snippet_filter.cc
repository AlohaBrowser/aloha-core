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

#include "components/adblock/core/converter/parser/snippet_filter.h"

#include "base/logging.h"

namespace adblock {

static constexpr char kDomainSeparator[] = ",";

// static
absl::optional<SnippetFilter> SnippetFilter::FromString(
    std::string_view domain_list,
    std::string_view snippet) {
  if (snippet.empty()) {
    VLOG(1) << "[eyeo] Filter has no snippet script.";
    return {};
  }

  DomainOption domains =
      DomainOption::FromString(domain_list, kDomainSeparator);
  // Snippet filters require that the domains have
  // at least one subdomain or is localhost
  domains.RemoveDomainsWithNoSubdomain();
  if (domains.GetIncludeDomains().empty()) {
    VLOG(1) << "Snippet "
               "filters require include domains.";
    return {};
  }

  auto snippet_script = SnippetTokenizer::Tokenize(snippet);
  if (snippet_script.empty()) {
    VLOG(1) << "Could not tokenize snippet script";
    return {};
  }

  return SnippetFilter(std::move(snippet_script), std::move(domains));
}

SnippetFilter::SnippetFilter(SnippetTokenizer::SnippetScript snippet_script,
                             DomainOption domains)
    : snippet_script(std::move(snippet_script)), domains(std::move(domains)) {}
SnippetFilter::SnippetFilter(const SnippetFilter& other) = default;
SnippetFilter::SnippetFilter(SnippetFilter&& other) = default;
SnippetFilter::~SnippetFilter() = default;

}  // namespace adblock
