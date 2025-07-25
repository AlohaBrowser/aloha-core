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

#include "components/adblock/core/converter/parser/filter_classifier.h"

#include "third_party/re2/src/re2/re2.h"

namespace adblock {

// static
FilterType FilterClassifier::Classify(std::string_view filter) {
  if (!filter.empty() && filter.back() == '}' &&
      (base::StartsWith(filter, kElemHideFilterSeparator) ||
       base::StartsWith(filter, kElemHideEmulationFilterSeparator))) {
    static re2::RE2 remove_re("\\{\\s*remove\\s*:\\s*true\\s*;\\s*\\}$");
    if (re2::RE2::PartialMatch(filter, remove_re)) {
      return FilterType::Remove;
    } else {
      return FilterType::InlineCss;
    }
  }
  if (base::StartsWith(filter, kElemHideFilterSeparator)) {
    return FilterType::ElemHide;
  }
  if (base::StartsWith(filter, kElemHideExceptionFilterSeparator)) {
    return FilterType::ElemHideException;
  }
  if (base::StartsWith(filter, kElemHideEmulationFilterSeparator)) {
    return FilterType::ElemHideEmulation;
  }
  if (base::StartsWith(filter, kSnippetFilterSeparator)) {
    return FilterType::Snippet;
  }

  return FilterType::Url;
}

}  // namespace adblock
