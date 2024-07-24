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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_FILTER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_FILTER_H_

#include <string_view>

#include "components/adblock/core/converter/parser/domain_option.h"
#include "components/adblock/core/converter/parser/snippet_tokenizer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace adblock {

class SnippetFilter {
 public:
  static absl::optional<SnippetFilter> FromString(std::string_view domain_list,
                                                  std::string_view snippet);

  SnippetFilter(const SnippetFilter& other);
  SnippetFilter(SnippetFilter&& other);
  ~SnippetFilter();

  const SnippetTokenizer::SnippetScript snippet_script;
  const DomainOption domains;

 private:
  SnippetFilter(SnippetTokenizer::SnippetScript snippet_script,
                DomainOption domains);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_FILTER_H_
