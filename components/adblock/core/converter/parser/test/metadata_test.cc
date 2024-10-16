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

#include "components/adblock/core/converter/parser/metadata.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockParserMetadataTest : public testing::Test {
 public:
  absl::optional<Metadata> ParseHeader(std::string header) {
    // Without [Adblock Plus 2.0], the file is not a valid filter list.
    header = "[Adblock Plus 2.0]\n" + header;
    std::stringstream input(std::move(header));
    return Metadata::FromStream(input);
  }
};

TEST_F(AdblockParserMetadataTest, EmptyInput) {
  std::stringstream input("");
  EXPECT_FALSE(Metadata::FromStream(input).has_value());
}

TEST_F(AdblockParserMetadataTest, NoMandatoryAdblockHeader) {
  std::stringstream input("! Title: EasyList");
  EXPECT_FALSE(Metadata::FromStream(input).has_value());
}

TEST_F(AdblockParserMetadataTest, InvalidAdblockHeader) {
  std::stringstream input("[Abdlock Puls]");
  EXPECT_FALSE(Metadata::FromStream(input).has_value());
}

TEST_F(AdblockParserMetadataTest, ShortestValidAdblockHeader) {
  std::stringstream input("[Adblock]");
  auto metadata = Metadata::FromStream(input);
  EXPECT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->homepage, "");
  EXPECT_FALSE(metadata->redirect_url.has_value());
  EXPECT_EQ(metadata->title, "");
  EXPECT_EQ(metadata->version, "");
  EXPECT_EQ(metadata->expires.InDays(),
            Metadata::kDefaultExpirationInterval.InDays());
}

TEST_F(AdblockParserMetadataTest, MandatoryAdblockHeaderPresent) {
  auto metadata = ParseHeader("");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->homepage, "");
  EXPECT_FALSE(metadata->redirect_url.has_value());
  EXPECT_EQ(metadata->title, "");
  EXPECT_EQ(metadata->version, "");
  EXPECT_EQ(metadata->expires.InDays(),
            Metadata::kDefaultExpirationInterval.InDays());
}

TEST_F(AdblockParserMetadataTest, HomepageIsSet) {
  auto metadata = ParseHeader("! Homepage: https://easylist.to/");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->homepage, "https://easylist.to/");
}

TEST_F(AdblockParserMetadataTest, RedirectUrlIsSet) {
  auto metadata = ParseHeader("! Redirect: https://redirect-easylist.to/");
  ASSERT_TRUE(metadata.has_value());
  ASSERT_TRUE(metadata->redirect_url.has_value());
  EXPECT_EQ(metadata->redirect_url.value().spec(),
            "https://redirect-easylist.to/");
}

TEST_F(AdblockParserMetadataTest, InvalidRedirectUrlIsNotSet) {
  auto metadata = ParseHeader("! Homepage: https//invalid_redirect_url.to/");
  ASSERT_TRUE(metadata.has_value());
  ASSERT_FALSE(metadata->redirect_url.has_value());
}

TEST_F(AdblockParserMetadataTest, TitleIsSet) {
  auto metadata = ParseHeader("! Title: EasyList");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->title, "EasyList");
}

TEST_F(AdblockParserMetadataTest, VersionIsSet) {
  auto metadata = ParseHeader("! Version: 202204050843");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->version, "202204050843");
}

TEST_F(AdblockParserMetadataTest, ExpirationTimeIsSetHoursExplicit) {
  auto metadata = ParseHeader("! Expires: 2 hours (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InHours(), 2);
}

TEST_F(AdblockParserMetadataTest, ExpirationTimeIsSetHoursImplicit) {
  auto metadata = ParseHeader("! Expires: 2 horse (sea)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InHours(), 2);
}

TEST_F(AdblockParserMetadataTest, ExpirationTimeIsSetHoursDoubleDigit) {
  auto metadata = ParseHeader("! Expires: 48 hours (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(), 2);
}

TEST_F(AdblockParserMetadataTest, ExpirationTimeIsSetDaysExplicit) {
  auto metadata = ParseHeader("! Expires: 2 days (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(), 2);
}

TEST_F(AdblockParserMetadataTest, ExpirationTimeIsSetDaysImplicit) {
  auto metadata = ParseHeader("! Expires: 2 not a horse");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(), 2);
}

TEST_F(AdblockParserMetadataTest, InvalidExpirationTimeNotParsed) {
  auto metadata = ParseHeader("! Expires: two days (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(),
            Metadata::kDefaultExpirationInterval.InDays());
}

TEST_F(AdblockParserMetadataTest, NegativeExpirationTimeNotParsed) {
  auto metadata = ParseHeader("! Expires: -1 days (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(),
            Metadata::kDefaultExpirationInterval.InDays());
}

TEST_F(AdblockParserMetadataTest, InvalidExpirationTimeUpperLimit) {
  auto metadata = ParseHeader("! Expires: 15");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InDays(),
            Metadata::kMaxExpirationInterval.InDays());
}

TEST_F(AdblockParserMetadataTest, InvalidExpirationTimeLowerLimit) {
  auto metadata = ParseHeader(
      "! Expires: 0 how do you throw a space party? You planet. (hours)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->expires.InHours(),
            Metadata::kMinExpirationInterval.InHours());
}

TEST_F(AdblockParserMetadataTest, AllMembersSet) {
  auto metadata = ParseHeader(
      "! Homepage: https://easylist.to/\n"
      "! Redirect: https://redirect-easylist.to/\n"
      "! Title: EasyList\n"
      "! Version: 202204050843\n"
      "! Expires: 2 hours (update frequency)");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->homepage, "https://easylist.to/");
  ASSERT_TRUE(metadata->redirect_url.has_value());
  EXPECT_EQ(metadata->redirect_url.value().spec(),
            "https://redirect-easylist.to/");
  EXPECT_EQ(metadata->title, "EasyList");
  EXPECT_EQ(metadata->version, "202204050843");
  EXPECT_EQ(metadata->expires.InMilliseconds(), 2 * 3600000);
}

TEST_F(AdblockParserMetadataTest, ParsingStopsAtFirstNotHeaderLine) {
  std::stringstream input(
      "[Adblock Plus 2.0]\n"
      "! Title: EasyList\n"
      "Definitely NOT a header line\n"
      "! Version: 202204050843");

  auto metadata = Metadata::FromStream(input);
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->title, "EasyList");
  EXPECT_EQ(metadata->version, "");

  // Reading next line from input returns first not header line
  std::string line;
  std::getline(input, line);
  EXPECT_EQ(line, "Definitely NOT a header line");
}

}  // namespace adblock
