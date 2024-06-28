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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_H_

#include <string>

#include "components/adblock/core/converter/parser/url_filter_options.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace adblock {

class UrlFilter {
 public:
  static absl::optional<UrlFilter> FromString(std::string filter_str);

  UrlFilter(const UrlFilter& other);
  UrlFilter(UrlFilter&& other);
  ~UrlFilter();

  const bool is_allowing;
  const std::string pattern;
  const UrlFilterOptions options;

 private:
  UrlFilter(bool is_allowing, std::string pattern, UrlFilterOptions options);

  static bool IsValidRegex(const std::string& pattern);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_URL_FILTER_H_
