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

#include "components/adblock/core/subscription/test/installed_subscription_impl_test_base.h"

#include "absl/types/variant.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

struct FlatIndex {
  explicit FlatIndex(std::unique_ptr<FlatbufferData> data)
      : buffer_(std::move(data)),
        index_(flat::GetSubscription(buffer_->data())) {}
  const std::unique_ptr<FlatbufferData> buffer_;
  const raw_ptr<const flat::Subscription> index_;
};

class AdblockInstalledSubscriptionImplSnippetsTest
    : public AdblockInstalledSubscriptionImplTestBase {
 public:
  FlatIndex ConvertAndLoadRulesToIndex(std::string rules,
                                       GURL url = GURL(),
                                       bool allow_privileged = false) {
    // Without [Adblock Plus 2.0], the file is not a valid filter list.
    rules = "[Adblock Plus 2.0]\n! Title: TestingList\n" + rules;
    std::stringstream input(std::move(rules));
    auto converter_result =
        FlatbufferConverter::Convert(input, url, allow_privileged);
    if (!absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
            converter_result)) {
      return FlatIndex(nullptr);
    }
    return FlatIndex(std::move(
        absl::get<std::unique_ptr<FlatbufferData>>(converter_result)));
  }
};

/* --------------------- Snippet tests --------------------- */
TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetEmpty) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#
    )",
                                          {}, true);
  EXPECT_EQ(index.index_->snippet()->size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetBasic) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com,~other.test.com#$#log Test
    )",
                                          {}, true);
  ASSERT_EQ(index.index_->snippet()->size(), 1u);
  auto* entry = index.index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  EXPECT_EQ(entry->domain()->str(), "test.com");

  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->exclude_domains()->size(), 1u);
  EXPECT_EQ(snippet->exclude_domains()->Get(0)->str(), "other.test.com");
  EXPECT_EQ(snippet->script()->size(), 1u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "Test");
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetSpace) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#log '   Test\t arg'
    )",
                                          {}, true);
  ASSERT_EQ(index.index_->snippet()->size(), 1u);
  auto* entry = index.index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "   Test\t arg");
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetArgumentPack) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#log ab 'a b' '' ccc
    )",
                                          {}, true);
  ASSERT_EQ(index.index_->snippet()->size(), 1u);
  auto* entry = index.index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->script()->size(), 1u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 4u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "ab");
  EXPECT_EQ(call->arguments()->Get(1)->str(), "a b");
  EXPECT_EQ(call->arguments()->Get(2)->str(), "");
  EXPECT_EQ(call->arguments()->Get(3)->str(), "ccc");
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetMultipleCalls) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#log test; log test; log
    )",
                                          {}, true);
  ASSERT_EQ(index.index_->snippet()->size(), 1u);
  auto* entry = index.index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->script()->size(), 3u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "test");

  call = snippet->script()->Get(1);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "test");

  call = snippet->script()->Get(2);
  EXPECT_EQ(call->command()->str(), "log");
  EXPECT_EQ(call->arguments()->size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, NoSnippetTest) {
  auto sub = ConvertAndLoadRules(R"(
    test.com##selector
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("random.org").size(), 0u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, NoSnippetForDomainTest) {
  auto sub = ConvertAndLoadRules(R"(
     test.com#$#log test; log test; log
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("domain.com").size(), 0u);
  EXPECT_EQ(sub->MatchSnippets("test.com").size(), 3u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest,
       SnippetFiltersTopLevelDomain) {
  auto sub = ConvertAndLoadRules(R"(
     test.com,gov.ua,com,localhost#$#abort-on-property-write 1 2 3
    )",
                                 {}, true);
  // has to have at least one subdomain
  // but it might be a Top Level domain like gov.ua
  EXPECT_EQ(sub->MatchSnippets("test.com").size(), 1u);
  EXPECT_EQ(sub->MatchSnippets("gov.ua").size(), 1u);
  EXPECT_EQ(sub->MatchSnippets("com").size(), 0u);
  EXPECT_EQ(sub->MatchSnippets("localhost").size(), 1u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest, SnippetFiltersSubdomain) {
  auto sub = ConvertAndLoadRules(R"(
     example.com,gov.ua#$#abort-on-property-write 1 2 3
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("www.example.com").size(), 1u);
}

TEST_F(AdblockInstalledSubscriptionImplSnippetsTest,
       SnippetsIgnoredForNonPriviledged) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     example.com,gov.ua#$#abort-on-property-write 1 2 3
    )",
                                          {}, false);
  ASSERT_EQ(index.index_->snippet()->size(), 0u);
}

}  // namespace adblock
