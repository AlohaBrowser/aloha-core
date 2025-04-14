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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_REGEX_MATCHER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_REGEX_MATCHER_H_

#include <map>
#include <string_view>

#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace adblock {

class RegexMatcher {
 public:
  RegexMatcher();
  ~RegexMatcher();
  RegexMatcher(const RegexMatcher&) = delete;
  RegexMatcher(RegexMatcher&&) = delete;
  RegexMatcher& operator=(const RegexMatcher&) = delete;
  RegexMatcher& operator=(RegexMatcher&&) = delete;

  // Max number of patterns that PreBuildRegexPatternsWithNoKeyword() will
  // build and store in memory. If there are more regex patterns in |index|,
  // they will not be pre-built and MatchesRegex() will build them on demand.
  static constexpr int kMaxPrebuiltPatterns = 500;

  // Regex patterns that have no keyword attached must be matched to every URL.
  // There are typically few of them and they are matched very often, so
  // pre-build them.
  void PreBuildRegexPatternsWithNoKeyword(const flat::Subscription* index);
  void PreBuildRegexPattern(std::string_view regular_expression,
                            bool case_sensitive);

  bool MatchesRegex(std::string_view regex_pattern,
                    const GURL& url,
                    bool case_sensitive) const;

 private:
  using UrlFilterIndex =
      flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>;
  void PreBuildPatternsFrom(const UrlFilterIndex* index);
  std::unique_ptr<re2::RE2> BuildRe2Expression(
      std::string_view regular_expression,
      bool case_sensitive);
  std::unique_ptr<icu::RegexPattern> BuildIcuExpression(
      std::string_view regular_expression,
      bool case_sensitive);
  int CacheSize() const;

  using CacheKey = std::tuple<std::string_view, bool>;
  std::map<CacheKey, std::unique_ptr<re2::RE2>> re2_cache_;
  std::map<CacheKey, std::unique_ptr<icu::RegexPattern>> icu_cache_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_REGEX_MATCHER_H_
