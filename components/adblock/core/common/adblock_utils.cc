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

#include <numeric>

#include "base/containers/flat_map.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/safe_sprintf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/version_info/version_info.h"
#include "net/http/http_response_headers.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace adblock {
namespace utils {
namespace {
constexpr char kLanguagesSeparator[] = ",";
}

std::string CreateDomainAllowlistingFilter(const std::string& domain) {
  return "@@||" + base::ToLowerASCII(domain) +
         "^$document,domain=" + base::ToLowerASCII(domain);
}

SiteKey GetSitekeyHeader(
    const scoped_refptr<net::HttpResponseHeaders>& headers) {
  size_t iterator = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&iterator, &name, &value)) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (name == adblock::kSiteKeyHeaderKey) {
      return SiteKey{value};
    }
  }
  return {};
}

AppInfo::AppInfo() = default;

AppInfo::~AppInfo() = default;

AppInfo::AppInfo(const AppInfo&) = default;

AppInfo GetAppInfo() {
  AppInfo info;

#if defined(EYEO_APPLICATION_NAME)
  info.name = EYEO_APPLICATION_NAME;
#else
  info.name = version_info::GetProductName();
#endif
#if defined(EYEO_APPLICATION_VERSION)
  info.version = EYEO_APPLICATION_VERSION;
#else
  info.version = version_info::GetVersionNumber();
#endif
  base::ReplaceChars(version_info::GetOSType(), base::kWhitespaceASCII, "",
                     &info.client_os);
  return info;
}

std::string SerializeLanguages(const std::vector<std::string> languages) {
  if (languages.empty()) {
    return {};
  }

  return std::accumulate(std::next(languages.begin()), languages.end(),
                         languages[0],
                         [](const std::string& a, const std::string& b) {
                           return a + kLanguagesSeparator + b;
                         });
}

std::vector<std::string> DeserializeLanguages(const std::string languages) {
  return base::SplitString(languages, kLanguagesSeparator,
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input) {
  std::vector<std::string> output;
  output.reserve(input.size());
  std::transform(std::begin(input), std::end(input), std::back_inserter(output),
                 [](const GURL& gurl) { return gurl.spec(); });
  return output;
}

std::string ConvertBaseTimeToABPFilterVersionFormat(const base::Time& date) {
  base::Time::Exploded exploded;
  // we receive in GMT and convert to UTC ( which has the same time )
  date.UTCExplode(&exploded);
  char buff[16];
  base::strings::SafeSPrintf(buff, "%04d%02d%02d%02d%02d", exploded.year,
                             exploded.month, exploded.day_of_month,
                             exploded.hour, exploded.minute);
  return std::string(buff);
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

}  // namespace utils
}  // namespace adblock
