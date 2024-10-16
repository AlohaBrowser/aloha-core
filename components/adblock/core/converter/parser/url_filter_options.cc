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

#include "components/adblock/core/converter/parser/url_filter_options.h"

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace adblock {

using SiteKeys = std::vector<SiteKey>;

static constexpr char kDomainOrSitekeySeparator[] = "|";
static constexpr char kInverseSymbol = '~';

// static
absl::optional<UrlFilterOptions> UrlFilterOptions::FromString(
    const std::string& option_list) {
  bool is_match_case = false;
  bool is_popup_filter = false;
  bool is_subresource = false;
  ThirdPartyOption third_party = ThirdPartyOption::Ignore;
  absl::optional<RewriteOption> rewrite;
  DomainOption domains;
  SiteKeys sitekeys;
  absl::optional<std::string> csp;
  absl::optional<std::string> headers;
  uint32_t content_types = 0;
  std::set<ExceptionType> exception_types;

  bool is_inverse_option;
  std::string key, value;
  for (auto& option : base::SplitString(option_list, ",", base::KEEP_WHITESPACE,
                                        base::SPLIT_WANT_NONEMPTY)) {
    if (option.empty()) {
      continue;
    }

    is_inverse_option = option.front() == kInverseSymbol;
    if (is_inverse_option) {
      option.erase(0, 1);
    }

    size_t delimiter_pos = option.find('=');
    if (delimiter_pos != std::string::npos) {
      key = option.substr(0, delimiter_pos);
      value = option.substr(delimiter_pos + 1);
    } else {
      key = option;
    }

    key = base::ToLowerASCII(key);
    base::RemoveChars(key, base::kWhitespaceASCII, &key);

    if (key == "match-case") {
      is_match_case = !is_inverse_option;
    } else if (key == "popup") {
      is_popup_filter = true;
    } else if (key == "third-party") {
      third_party = !is_inverse_option ? ThirdPartyOption::ThirdPartyOnly
                                       : ThirdPartyOption::FirstPartyOnly;
    } else if (key == "rewrite") {
      rewrite = ParseRewrite(value);
      if (!rewrite.has_value()) {
        VLOG(1) << "[eyeo] Invalid rewrite filter value: " << value;
        return {};
      }
    } else if (key == "domain") {
      if (value.empty()) {
        VLOG(1) << "[eyeo] Domain option has to have a value.";
        return {};
      }
      domains = DomainOption::FromString(value, kDomainOrSitekeySeparator);
    } else if (key == "sitekey") {
      if (value.empty()) {
        VLOG(1) << "[eyeo] Sitekey option has to have a value.";
        return {};
      }
      sitekeys = ParseSitekeys(value);
    } else if (key == "csp") {
      if (!IsValidCsp(value)) {
        VLOG(1) << "[eyeo] Invalid CSP filter directives: " << value;
        return {};
      }
      csp = value;
    } else if (key == "header") {
      ParseHeaders(value);
      headers = value;
    } else {
      ContentType content_type = ContentTypeFromString(key);
      if (content_type != ContentType::Unknown) {
        is_subresource = true;
        if (is_inverse_option) {
          if (content_types == 0) {
            content_types = ContentType::Default;
          }

          content_types &= ~content_type;
        } else {
          content_types |= content_type;
        }
        continue;
      }

      auto exception_type = ExceptionTypeFromString(key);
      if (exception_type) {
        // NOTE: Inverse exception types are not supported
        exception_types.emplace(exception_type.value());
        continue;
      }

      VLOG(1) << "[eyeo] Unknown filter option: " << key;
      return {};
    }
  }

  if (exception_types.empty() && !is_popup_filter && !csp.has_value() &&
      !rewrite.has_value() && !headers.has_value()) {
    is_subresource = true;
  }

  if (content_types == 0) {
    content_types = ContentType::Default;
  }

  return UrlFilterOptions(is_match_case, is_popup_filter, is_subresource,
                          third_party, content_types, std::move(rewrite),
                          std::move(domains), std::move(sitekeys),
                          std::move(csp), std::move(headers),
                          std::move(exception_types));
}

UrlFilterOptions::UrlFilterOptions()
    : is_match_case_(false),
      is_popup_filter_(false),
      is_subresource_(true),
      third_party_(ThirdPartyOption::Ignore),
      content_types_(ContentType::Default) {}

// static
absl::optional<UrlFilterOptions::RewriteOption> UrlFilterOptions::ParseRewrite(
    const std::string& rewrite_value) {
  if (rewrite_value == "abp-resource:blank-text") {
    return RewriteOption::AbpResource_BlankText;
  } else if (rewrite_value == "abp-resource:blank-css") {
    return RewriteOption::AbpResource_BlankCss;
  } else if (rewrite_value == "abp-resource:blank-js") {
    return RewriteOption::AbpResource_BlankJs;
  } else if (rewrite_value == "abp-resource:blank-html") {
    return RewriteOption::AbpResource_BlankHtml;
  } else if (rewrite_value == "abp-resource:blank-mp3") {
    return RewriteOption::AbpResource_BlankMp3;
  } else if (rewrite_value == "abp-resource:blank-mp4") {
    return RewriteOption::AbpResource_BlankMp4;
  } else if (rewrite_value == "abp-resource:1x1-transparent-gif") {
    return RewriteOption::AbpResource_TransparentGif1x1;
  } else if (rewrite_value == "abp-resource:2x2-transparent-png") {
    return RewriteOption::AbpResource_TransparentPng2x2;
  } else if (rewrite_value == "abp-resource:3x2-transparent-png") {
    return RewriteOption::AbpResource_TransparentPng3x2;
  } else if (rewrite_value == "abp-resource:32x32-transparent-png") {
    return RewriteOption::AbpResource_TransparentPng32x32;
  } else {
    return {};
  }
}

// static
SiteKeys UrlFilterOptions::ParseSitekeys(const std::string& sitekey_value) {
  SiteKeys sitekeys;
  for (auto& sitekey : base::SplitString(
           base::ToUpperASCII(sitekey_value), kDomainOrSitekeySeparator,
           base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    sitekeys.emplace_back(std::move(sitekey));
  }
  std::sort(sitekeys.begin(), sitekeys.end());
  return sitekeys;
}

// static
bool UrlFilterOptions::IsValidCsp(const std::string& csp_value) {
  static re2::RE2 invalid_csp(
      "(;|^) "
      "?(base-uri|referrer|report-to|report-uri|upgrade-insecure-requests)\\b");

  return !(re2::RE2::PartialMatch(
      re2::StringPiece(csp_value.data(), csp_value.size()), invalid_csp));
}

// static
void UrlFilterOptions::ParseHeaders(std::string& headers_value) {
  // replace \x2c with actual ,
  static re2::RE2 r1("([^\\\\])\\\\x2c");
  re2::RE2::GlobalReplace(&headers_value, r1, "\\1,");

  // remove extra escape for \\x2c which left
  static re2::RE2 r2("\\\\x2c");
  re2::RE2::GlobalReplace(&headers_value, r2, "x2c");
}

// static
absl::optional<UrlFilterOptions::ExceptionType>
UrlFilterOptions::ExceptionTypeFromString(const std::string& exception_type) {
  if (exception_type == "document") {
    return ExceptionType::Document;
  } else if (exception_type == "genericblock") {
    return ExceptionType::Genericblock;
  } else if (exception_type == "elemhide") {
    return ExceptionType::Elemhide;
  } else if (exception_type == "generichide") {
    return ExceptionType::Generichide;
  }
  return {};
}

UrlFilterOptions::UrlFilterOptions(
    const bool is_match_case,
    const bool is_popup_filter,
    const bool is_subresource,
    const ThirdPartyOption third_party,
    const uint32_t content_types,
    const absl::optional<RewriteOption> rewrite,
    const DomainOption domains,
    const SiteKeys sitekeys,
    const absl::optional<std::string> csp,
    const absl::optional<std::string> headers,
    const std::set<ExceptionType> exception_types)
    : is_match_case_(is_match_case),
      is_popup_filter_(is_popup_filter),
      is_subresource_(is_subresource),
      third_party_(third_party),
      content_types_(content_types),
      rewrite_(std::move(rewrite)),
      domains_(std::move(domains)),
      sitekeys_(std::move(sitekeys)),
      csp_(std::move(csp)),
      headers_(std::move(headers)),
      exception_types_(std::move(exception_types)) {}
UrlFilterOptions::UrlFilterOptions(const UrlFilterOptions& other) = default;
UrlFilterOptions::UrlFilterOptions(UrlFilterOptions&& other) = default;
UrlFilterOptions& UrlFilterOptions::operator=(const UrlFilterOptions& other) =
    default;
UrlFilterOptions& UrlFilterOptions::operator=(UrlFilterOptions&& other) =
    default;
UrlFilterOptions::~UrlFilterOptions() = default;

}  // namespace adblock
