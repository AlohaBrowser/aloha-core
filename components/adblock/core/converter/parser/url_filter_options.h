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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_OPTIONS_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_OPTIONS_H_

#include <set>
#include <string>
#include <vector>

#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/converter/parser/domain_option.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace adblock {

class UrlFilterOptions {
 public:
  enum class ThirdPartyOption {
    Ignore,
    ThirdPartyOnly,
    FirstPartyOnly,
  };

  enum class RewriteOption {
    AbpResource_BlankText,
    AbpResource_BlankCss,
    AbpResource_BlankJs,
    AbpResource_BlankHtml,
    AbpResource_BlankMp3,
    AbpResource_BlankMp4,
    AbpResource_TransparentGif1x1,
    AbpResource_TransparentPng2x2,
    AbpResource_TransparentPng3x2,
    AbpResource_TransparentPng32x32,
  };

  enum class ExceptionType {
    Document,
    Elemhide,
    Generichide,
    Genericblock,
  };

  static absl::optional<UrlFilterOptions> FromString(
      const std::string& option_list);

  UrlFilterOptions();
  UrlFilterOptions(const UrlFilterOptions& other);
  UrlFilterOptions(UrlFilterOptions&& other);
  UrlFilterOptions& operator=(const UrlFilterOptions& other);
  UrlFilterOptions& operator=(UrlFilterOptions&& other);
  ~UrlFilterOptions();

  inline bool IsMatchCase() const { return is_match_case_; }
  inline bool IsPopup() const { return is_popup_filter_; }
  inline bool IsSubresource() const { return is_subresource_; }
  inline ThirdPartyOption ThirdParty() const { return third_party_; }
  inline const absl::optional<RewriteOption>& Rewrite() const {
    return rewrite_;
  }
  inline const DomainOption& Domains() const { return domains_; }
  inline const std::vector<SiteKey>& Sitekeys() const { return sitekeys_; }
  inline const absl::optional<std::string>& Csp() const { return csp_; }
  inline const absl::optional<std::string>& Headers() const { return headers_; }
  inline uint32_t ContentTypes() const { return content_types_; }
  inline const std::set<ExceptionType>& ExceptionTypes() const {
    return exception_types_;
  }

 private:
  UrlFilterOptions(bool is_match_case,
                   bool is_popup_filter,
                   bool is_subresource,
                   ThirdPartyOption third_party,
                   uint32_t content_types,
                   absl::optional<RewriteOption> rewrite,
                   DomainOption domains,
                   std::vector<SiteKey> sitekeys,
                   absl::optional<std::string> csp,
                   absl::optional<std::string> headers,
                   std::set<ExceptionType> exception_types);

  static absl::optional<RewriteOption> ParseRewrite(
      const std::string& rewrite_value);
  static std::vector<SiteKey> ParseSitekeys(const std::string& sitekey_value);
  static bool IsValidCsp(const std::string& csp_value);
  static void ParseHeaders(std::string& headers_value);
  static absl::optional<ExceptionType> ExceptionTypeFromString(
      const std::string& exception_type);

  bool is_match_case_;
  bool is_popup_filter_;
  bool is_subresource_;
  ThirdPartyOption third_party_;
  uint32_t content_types_;
  absl::optional<RewriteOption> rewrite_;
  DomainOption domains_;
  std::vector<SiteKey> sitekeys_;
  absl::optional<std::string> csp_;
  absl::optional<std::string> headers_;
  std::set<ExceptionType> exception_types_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_OPTIONS_H_
