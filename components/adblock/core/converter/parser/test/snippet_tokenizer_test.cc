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

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using ::testing::ElementsAreArray;

class AdblockSnippetTokenizerTest : public testing::Test {
 public:
  void Validate(const std::string& input,
                const SnippetTokenizer::SnippetScript& output) {
    auto script = SnippetTokenizer::Tokenize(input);
    ASSERT_EQ(script.size(), output.size());
    for (size_t n = 0, cnt = script.size(); n != cnt; ++n) {
      EXPECT_THAT(script[n], ElementsAreArray(output[n]));
    }
  }
};

TEST_F(AdblockSnippetTokenizerTest, NoArguments) {
  Validate("foo", {{"foo"}});
}

TEST_F(AdblockSnippetTokenizerTest, OneArgument) {
  Validate("foo 1", {{"foo", "1"}});
}

TEST_F(AdblockSnippetTokenizerTest, TwoArguments) {
  Validate("foo 1 Hello", {{"foo", "1", "Hello"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentWithSpace) {
  Validate("foo Hello\\ world", {{"foo", "Hello world"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentQuoted) {
  Validate("foo 'Hello world'", {{"foo", "Hello world"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentWithQuote) {
  Validate("foo 'Hello \\'world\\''", {{"foo", "Hello 'world'"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentWithEscapedSemicolon) {
  Validate("foo TL\\;DR", {{"foo", "TL;DR"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentWithQuotedSemicolon) {
  Validate("foo 'TL;DR'", {{"foo", "TL;DR"}});
}

TEST_F(AdblockSnippetTokenizerTest, ArgumentWithEscapeSequences) {
  Validate("foo yin\\tyang\\n", {{"foo", "yin\tyang\n"}});
}

TEST_F(AdblockSnippetTokenizerTest, MultipleCommands) {
  Validate("foo; bar", {{"foo"}, {"bar"}});
}

TEST_F(AdblockSnippetTokenizerTest, MultipleCommandsWithArguments) {
  Validate("foo 1 Hello; bar world! #",
           {{"foo", "1", "Hello"}, {"bar", "world!", "#"}});
}

TEST_F(AdblockSnippetTokenizerTest, MultipleCommandsWithQuotation) {
  Validate(
      "foo 1 'Hello, \\'Tommy\\'!' ;bar Hi!\\ How\\ are\\ you? "
      "http://example.com",
      {{"foo", "1", "Hello, 'Tommy'!"},
       {"bar", "Hi! How are you?", "http://example.com"}});
}

TEST_F(AdblockSnippetTokenizerTest, SpecialCharacters) {
  Validate("fo\\'\\ \\ \\\t\\\n\\;o 1 2 3; 'b a  r' 1 2",
           {{"fo'  \t\n;o", "1", "2", "3"}, {"b a  r", "1", "2"}});
}

TEST_F(AdblockSnippetTokenizerTest, UnicodeStaysAsIs) {
  Validate("foo \\u0062\\ud83d\\ude42r", {{"foo", "\\u0062\\ud83d\\ude42r"}});
}

TEST_F(AdblockSnippetTokenizerTest, MultipleCommandsNoArguments) {
  Validate("foo; ;;; ;  ; bar 1", {{"foo"}, {"bar", "1"}});
}

TEST_F(AdblockSnippetTokenizerTest, MultipleCommandsBlankArguments) {
  Validate("foo '' ''", {{"foo", "", ""}});
}

TEST_F(AdblockSnippetTokenizerTest, QuotedSpaceWithinArgument) {
  Validate("foo Hello' 'world", {{"foo", "Hello world"}});
}

TEST_F(AdblockSnippetTokenizerTest, QuotedComaWithinArgument) {
  Validate("foo Hello','world", {{"foo", "Hello,world"}});
}

TEST_F(AdblockSnippetTokenizerTest, QuotedCharsWithinArgument) {
  Validate("foo Hello', 'world", {{"foo", "Hello, world"}});
}

TEST_F(AdblockSnippetTokenizerTest, QuotedStartOfArgument) {
  Validate("foo 'Hello, 'world", {{"foo", "Hello, world"}});
}

TEST_F(AdblockSnippetTokenizerTest, QuotedEndOfArgument) {
  Validate("foo Hello', world'", {{"foo", "Hello, world"}});
}

TEST_F(AdblockSnippetTokenizerTest, NoClosingQuote) {
  Validate("foo 'Hello, world", {});
}

TEST_F(AdblockSnippetTokenizerTest, NoClosingQuoteLastCommand) {
  Validate("foo Hello; bar 'How are you?", {{"foo", "Hello"}});
}

TEST_F(AdblockSnippetTokenizerTest, EndingWithBackslash) {
  Validate("foo Hello; bar 'How are you?' \\", {{"foo", "Hello"}});
}

}  // namespace adblock
