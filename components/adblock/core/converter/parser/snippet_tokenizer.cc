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

#include "components/adblock/core/converter/parser/snippet_tokenizer.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/third_party/icu/icu_utf.h"

namespace adblock {

SnippetTokenizer::SnippetScript SnippetTokenizer::Tokenize(
    std::string_view input) {
  SnippetScript script;
  std::string token;
  std::vector<std::string> arguments;
  bool escape = false;
  bool quotes_just_closed = false;
  bool within_quotes = false;
  for (char ch : input) {
    if (escape) {
      AddEscapeChar(token, ch);
      escape = false;
      quotes_just_closed = false;
    } else if (ch == '\\') {
      escape = true;
      quotes_just_closed = false;
    } else if (ch == '\'') {
      within_quotes = !within_quotes;
      quotes_just_closed = !within_quotes;
    } else if (within_quotes || (ch != ';' && !base::IsAsciiWhitespace(ch))) {
      token += ch;
      quotes_just_closed = false;
    } else {
      AddArgument(arguments, token, quotes_just_closed);
      if (ch == ';') {
        AddFunctionCall(script, arguments, within_quotes, escape);
      }
      quotes_just_closed = false;
    }
  }
  AddArgument(arguments, token, quotes_just_closed);
  AddFunctionCall(script, arguments, within_quotes, escape);
  return script;
}

void SnippetTokenizer::AddEscapeChar(std::string& token, char ch) {
  switch (ch) {
    case 'r':
      token += '\r';
      break;
    case 'n':
      token += '\n';
      break;
    case 't':
      token += '\t';
      break;
    case 'u':
      token += "\\u";
      break;
    default:
      token += ch;
  }
}

void SnippetTokenizer::AddArgument(std::vector<std::string>& arguments,
                                   std::string& token,
                                   bool quotes_just_closed) {
  if (quotes_just_closed || !token.empty()) {
    arguments.push_back(token);
    token.clear();
  }
}

void SnippetTokenizer::AddFunctionCall(SnippetScript& script,
                                       std::vector<std::string>& arguments,
                                       bool within_quotes,
                                       bool escape) {
  // if within quote whole script is invalid
  // or if detected escape char but ended
  if (arguments.empty() || within_quotes || escape) {
    return;
  }
  script.push_back(arguments);
  arguments.clear();
}

}  // namespace adblock
