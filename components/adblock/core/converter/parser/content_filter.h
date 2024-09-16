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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_CONTENT_FILTER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_CONTENT_FILTER_H_

#include <string>
#include <string_view>

#include "components/adblock/core/converter/parser/domain_option.h"
#include "components/adblock/core/converter/parser/filter_classifier.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace adblock {

class ContentFilter {
 public:
  static absl::optional<ContentFilter> FromString(std::string_view domain_list,
                                                  FilterType type,
                                                  std::string_view selector);

  ContentFilter(const ContentFilter& other);
  ~ContentFilter();

  const FilterType type;
  const std::string selector;
  const std::string modifier;
  const DomainOption domains;

 private:
  ContentFilter(FilterType type,
                std::string_view selector,
                std::string_view modifier,
                DomainOption domains);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_CONTENT_FILTER_H_
