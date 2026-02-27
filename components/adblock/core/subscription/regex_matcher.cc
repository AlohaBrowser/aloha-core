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

#include "components/adblock/core/subscription/regex_matcher.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/regex_filter_pattern.h"
#include "re2/re2.h"
#include "re2/stringpiece.h"
#include "third_party/re2/src/re2/re2.h"
#include "unicode/utypes.h"

namespace adblock {

RegexMatcher::RegexMatcher() = default;
RegexMatcher::~RegexMatcher() = default;

void RegexMatcher::PreBuildRegexPatternsWithNoKeyword(
    const flat::Subscription* index) {
  base::ElapsedTimer timer;
  PreBuildPatternsFrom(index->url_csp_allow());
  PreBuildPatternsFrom(index->url_csp_block());
  PreBuildPatternsFrom(index->url_document_allow());
  PreBuildPatternsFrom(index->url_genericblock_allow());
  PreBuildPatternsFrom(index->url_header_allow());
  PreBuildPatternsFrom(index->url_popup_allow());
  PreBuildPatternsFrom(index->url_popup_block());
  PreBuildPatternsFrom(index->url_elemhide_allow());
  PreBuildPatternsFrom(index->url_rewrite_allow());
  PreBuildPatternsFrom(index->url_rewrite_block());
  PreBuildPatternsFrom(index->url_subresource_allow());
  PreBuildPatternsFrom(index->url_subresource_block());
  VLOG(1) << "Added " << CacheSize() << " precompiled regular expressions in "
          << timer.Elapsed();
}

void RegexMatcher::PreBuildRegexPattern(std::string_view regular_expression,
                                        bool case_sensitive) {
  auto re2_pattern = BuildRe2Expression(regular_expression, case_sensitive);
  if (re2_pattern) {
    re2_cache_[std::make_pair(regular_expression, case_sensitive)] =
        std::move(re2_pattern);
  } else {
    auto icu_pattern = BuildIcuExpression(regular_expression, case_sensitive);
    if (!icu_pattern) {
      LOG(ERROR) << "Even ICU cannot parse this regular expression, "
                    "this should have been caught during parsing. Will "
                    "ignore this filter: "
                 << regular_expression;
      return;
    }
    icu_cache_[std::make_pair(regular_expression, case_sensitive)] =
        std::move(icu_pattern);
  }
}

bool RegexMatcher::MatchesRegex(std::string_view regex_pattern,
                                const GURL& url,
                                bool case_sensitive) const {
  const std::string_view input = url.spec();
  const auto cache_key = std::make_pair(regex_pattern, case_sensitive);

  const auto cached_re2_expression = re2_cache_.find(cache_key);
  if (cached_re2_expression != re2_cache_.end()) {
    return re2::RE2::PartialMatch(input.data(), *cached_re2_expression->second);
  }

  const auto cached_icu_expression = icu_cache_.find(cache_key);
  if (cached_icu_expression != icu_cache_.end()) {
    const icu::UnicodeString icu_input(input.data(), input.length());
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::RegexMatcher> regex_matcher = base::WrapUnique(
        cached_icu_expression->second->matcher(icu_input, status));
    bool is_match = regex_matcher->find(0, status);
    DCHECK(U_SUCCESS(status));
    return is_match;
  }
  VLOG(1) << "Matching a non-prebuilt expression, this will be slow";
  return utils::RegexMatches(regex_pattern, input, case_sensitive);
}

void RegexMatcher::PreBuildPatternsFrom(const UrlFilterIndex* index) {
  if (!index) {
    return;
  }
  const auto* idx = index->LookupByKey("");
  if (!idx) {
    return;
  }
  for (const auto* filter : *(idx->filter())) {
    if (CacheSize() >= kMaxPrebuiltPatterns) {
      return;
    }
    if (!filter->pattern()) {
      continue;  // This filter has no keyword because it has an empty pattern.
    }
    const std::string_view filter_string(filter->pattern()->c_str(),
                                         filter->pattern()->size());
    const auto regex_string = ExtractRegexFilterFromPattern(filter_string);
    if (!regex_string) {
      continue;  // This is not a regex filter.
    }
    PreBuildRegexPattern(*regex_string, filter->match_case());
  }
}

std::unique_ptr<re2::RE2> RegexMatcher::BuildRe2Expression(
    std::string_view regular_expression,
    bool case_sensitive) {
  re2::RE2::Options options;
  options.set_case_sensitive(case_sensitive);
  options.set_never_capture(true);
  options.set_log_errors(false);
  options.set_encoding(re2::RE2::Options::EncodingLatin1);
  auto prebuilt_re2 = std::make_unique<re2::RE2>(
      re2::StringPiece(regular_expression.data(), regular_expression.size()),
      options);
  if (!prebuilt_re2->ok()) {
    return nullptr;
  }
  return prebuilt_re2;
}

std::unique_ptr<icu::RegexPattern> RegexMatcher::BuildIcuExpression(
    std::string_view regular_expression,
    bool case_sensitive) {
  const icu::UnicodeString icu_pattern(regular_expression.data(),
                                       regular_expression.length());
  UErrorCode status = U_ZERO_ERROR;
  const auto icu_case_sensetive = case_sensitive ? 0u : UREGEX_CASE_INSENSITIVE;
  std::unique_ptr<icu::RegexPattern> prebuilt_pattern = base::WrapUnique(
      icu::RegexPattern::compile(icu_pattern, icu_case_sensetive, status));
  if (U_FAILURE(status) || !prebuilt_pattern) {
    return nullptr;
  }
  return prebuilt_pattern;
}

int RegexMatcher::CacheSize() const {
  return re2_cache_.size() + icu_cache_.size();
}

}  // namespace adblock
