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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_FILTER_CLASSIFIER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_FILTER_CLASSIFIER_H_

#include <string_view>

#include "base/strings/string_util.h"

namespace adblock {

static constexpr char kElemHideFilterSeparator[] = "##";
static constexpr char kElemHideExceptionFilterSeparator[] = "#@#";
static constexpr char kElemHideEmulationFilterSeparator[] = "#?#";
static constexpr char kSnippetFilterSeparator[] = "#$#";

enum class FilterType {
  ElemHide,
  ElemHideException,
  ElemHideEmulation,
  Remove,
  InlineCss,
  Snippet,
  Url,
};

class FilterClassifier {
 public:
  static FilterType Classify(std::string_view filter);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_FILTER_CLASSIFIER_H_
