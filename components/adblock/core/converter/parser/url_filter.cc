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

#include "components/adblock/core/converter/parser/url_filter.h"

#include <string_view>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/common/regex_filter_pattern.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"

namespace adblock {
namespace {

// Converts patterns like ||abc.com/aa|bb into ||abc.com/aa%7Cbb.
// Non-anchor pipe characters must be escaped to properly match components of
// a URL. GURL escapes "|" as seen in url/url_canon_path.cc
// Pipe characters in anchor position (beginning & end) must be left as-is, in
// order for the tokenizer to see them as anchors.
std::string SanitizePipeCharacters(std::string pattern) {
  auto piece = std::string_view(pattern);
  // Skip up to 2 leading | characters, they are treated as anchors. These
  // may not be replaced by the escaped variant.
  int number_of_left_anchors = 0;
  if (base::StartsWith(piece, "|")) {
    number_of_left_anchors++;
    piece.remove_prefix(1);
  }
  if (base::StartsWith(piece, "|")) {
    number_of_left_anchors++;
    piece.remove_prefix(1);
  }
  // Skip up to one trailing | characters, this is the right anchor.
  bool pattern_has_right_anchor = base::EndsWith(piece, "|");
  if (pattern_has_right_anchor) {
    piece.remove_suffix(1);
  }
  if (piece.find('|') == std::string_view::npos) {
    // The most common case, pattern has no pipe characters apart from anchors.
    // Avoid allocating new strings, pass the input out.
    return pattern;
  }
  // Escape instances of | the same way GURL does it.
  std::string output;
  CHECK(base::ReplaceChars(piece, "|", R"(%7C)", &output));
  // Re-add the unmodified anchors.
  for (int i = 0; i < number_of_left_anchors; i++) {
    output.insert(output.begin(), '|');
  }
  if (pattern_has_right_anchor) {
    output.push_back('|');
  }
  return output;
}

}  // namespace

static constexpr char kAllowingSymbol[] = "@@";
static constexpr char kOptionSymbol = '$';

bool IsGenericFilterIsNotSpecificEnough(
    std::string_view filter_str,
    const absl::optional<UrlFilterOptions>& options) {
  if (options.has_value() && (!options->Domains().GetExcludeDomains().empty() ||
                              !options->Domains().GetIncludeDomains().empty() ||
                              !options->Sitekeys().empty())) {
    return false;
  }
  const size_t kMinLength = 4;
  const auto trimmed_filter_str =
      base::TrimString(filter_str, "|", base::TRIM_LEADING);
  return trimmed_filter_str.size() < kMinLength &&
         trimmed_filter_str.find('*') == std::string::npos;
}

// static
absl::optional<UrlFilter> UrlFilter::FromString(std::string filter_str) {
  absl::optional<UrlFilterOptions> options;
  bool is_allowing = base::StartsWith(filter_str, kAllowingSymbol);
  if (is_allowing) {
    filter_str.erase(0, 2);
  }

  // TODO(DPD-1277): Support filters that contain multiple '$'
  size_t option_selector_it = filter_str.rfind(kOptionSymbol);
  if (option_selector_it != std::string::npos &&
      !ExtractRegexFilterFromPattern(filter_str)) {
    std::string option_list = filter_str.substr(option_selector_it + 1);
    options = UrlFilterOptions::FromString(option_list);

    if (!options.has_value()) {
      return {};
    }

    if (options->Csp().has_value() && options->Csp().value().empty() &&
        !is_allowing) {
      VLOG(1) << "[eyeo] Invalid CSP filter. Blocking CSP filter requires "
                 "directives";
      return {};
    }

    if (options->Headers().has_value() && options->Headers().value().empty() &&
        !is_allowing) {
      VLOG(1) << "[eyeo] Invalid header filter. Blocking header filter "
                 "requires directives";
      return {};
    }

    if (!options->IsSubresource() && !options->ExceptionTypes().empty() &&
        !is_allowing) {
      VLOG(1) << "[eyeo] Exception options can only be used with allowing "
                 "filters";
      return {};
    }

    filter_str.erase(option_selector_it);
  }

  if (filter_str.empty() && !options.has_value()) {
    return {};
  }

  if (!ExtractRegexFilterFromPattern(filter_str)) {
    // It's rare, but some filters contain pipe characters ("|") that are not
    // anchors but are instead integral parts of the URL they intend to match.
    // GURL escapes "|"" characters and we need to similarly escape such
    // occurrences in the filter.
    filter_str = SanitizePipeCharacters(std::move(filter_str));

    // Most filters are case-insensitive, we may lowercase them along with
    // lowercasing the URL during matching. This simplifies and speeds up the
    // matching algorithm. Do not lowercase case-sensitive filters.
    if ((!options || !options->IsMatchCase())) {
      filter_str = base::ToLowerASCII(filter_str);
    }
  }

  if (options.has_value() && options->Rewrite().has_value()) {
    if (options->ThirdParty() ==
        UrlFilterOptions::ThirdPartyOption::ThirdPartyOnly) {
      VLOG(1) << "[eyeo] Rewrite filter must not be used together with the "
                 "third-party filter option";
      return {};
    }

    if (!base::StartsWith(filter_str, "||") && filter_str != "*" &&
        !filter_str.empty()) {
      VLOG(1) << "[eyeo] Rewrite filter pattern must either be a star (*) "
                 "or start with a domain anchor double pipe (||)";
      return {};
    }

    if (options->Domains().GetIncludeDomains().empty() &&
        options->ThirdParty() == UrlFilterOptions::ThirdPartyOption::Ignore) {
      VLOG(1) << "[eyeo] Rewrite filter must be restricted to at least one "
                 "domain using the domain filter option or have ~third-party "
                 "option";
      return {};
    }
  }

  if (IsGenericFilterIsNotSpecificEnough(filter_str, options)) {
    VLOG(1) << "[eyeo] Generic url filter is not specific enough. Must be "
               "longer than 3 characters or domain-specific.";
    return {};
  }

  if (!options.has_value()) {
    options = UrlFilterOptions();
  }

  return UrlFilter(is_allowing, std::move(filter_str),
                   std::move(options.value()));
}

UrlFilter::UrlFilter(bool is_allowing,
                     std::string pattern,
                     UrlFilterOptions options)
    : is_allowing(is_allowing),
      pattern(std::move(pattern)),
      options(std::move(options)) {}
UrlFilter::UrlFilter(const UrlFilter& other) = default;
UrlFilter::UrlFilter(UrlFilter&& other) = default;
UrlFilter::~UrlFilter() = default;

// static
bool UrlFilter::IsValidRegex(const std::string& pattern) {
  re2::RE2::Options options;
  options.set_never_capture(true);
  options.set_log_errors(false);
  const re2::RE2 re2_pattern(pattern.data(), options);
  if (re2_pattern.ok()) {
    return true;
  }
  const icu::UnicodeString icu_pattern(pattern.data(), pattern.length());
  UErrorCode status = U_ZERO_ERROR;
  const icu::RegexMatcher matcher(icu_pattern, 0u, status);
  if (U_FAILURE(status)) {
    return false;
  }
  return true;
}

}  // namespace adblock
