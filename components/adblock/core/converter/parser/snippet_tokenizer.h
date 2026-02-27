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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_TOKENIZER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_TOKENIZER_H_

#include <string>
#include <string_view>
#include <vector>

namespace adblock {

class SnippetTokenizer {
 public:
  using SnippetScript = std::vector<std::vector<std::string>>;

  static SnippetScript Tokenize(std::string_view input);

 private:
  static void AddEscapeChar(std::string& token, char ch);
  static void AddArgument(std::vector<std::string>& arguments,
                          std::string& token,
                          bool quotes_just_closed);
  static void AddFunctionCall(SnippetScript& script,
                              std::vector<std::string>& arguments,
                              bool within_quotes,
                              bool escape);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_SNIPPET_TOKENIZER_H_
