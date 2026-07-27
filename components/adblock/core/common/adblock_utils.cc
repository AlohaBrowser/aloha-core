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

#include "components/adblock/core/common/adblock_utils.h"

#include "base/logging.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/resource/resource_bundle.h"

namespace adblock::utils {

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input) {
  std::vector<std::string> output;
  output.reserve(input.size());
  std::transform(std::begin(input), std::end(input), std::back_inserter(output),
                 [](const GURL& gurl) { return gurl.spec(); });
  return output;
}

std::unique_ptr<FlatbufferData> MakeFlatbufferDataFromResourceBundle(
    int resource_id) {
  return std::make_unique<InMemoryFlatbufferData>(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          resource_id));
}

bool RegexMatches(std::string_view pattern,
                  std::string_view input,
                  bool case_sensitive) {
  re2::RE2::Options options;
  options.set_case_sensitive(case_sensitive);
  options.set_never_capture(true);
  options.set_log_errors(false);
  options.set_encoding(re2::RE2::Options::EncodingLatin1);
  const re2::RE2 re2_pattern(pattern.data(), options);
  if (re2_pattern.ok()) {
    return re2::RE2::PartialMatch(input.data(), re2_pattern);
  }
  VLOG(2) << "[eyeo] RE2 does not support filter pattern " << pattern
          << " and return with error message: " << re2_pattern.error();

  // Maximum length of the string to match to avoid causing an icu::RegexMatcher
  // stack overflow. (crbug.com/1198219)
  if (input.size() > url::kMaxURLChars) {
    return false;
  }
  const icu::UnicodeString icu_pattern(pattern.data(), pattern.length());
  const icu::UnicodeString icu_input(input.data(), input.length());
  UErrorCode status = U_ZERO_ERROR;
  const auto icu_case_sensetive = case_sensitive ? 0u : UREGEX_CASE_INSENSITIVE;
  icu::RegexMatcher matcher(icu_pattern, icu_case_sensetive, status);

  // is pattern supported by icu regex
  if (U_FAILURE(status)) {
    // should not happen as validation should take place before reaching
    // this point
    DLOG(ERROR) << "[eyeo] None of the regex engines can use pattern: "
                << pattern;
    return false;
  }
  matcher.reset(icu_input);
  return matcher.find(0, status);
}

}  // namespace adblock::utils
