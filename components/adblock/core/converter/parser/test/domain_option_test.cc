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

#include "components/adblock/core/converter/parser/domain_option.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using ::testing::UnorderedElementsAre;

TEST(AdblockDomainOptionTest, ParseDomainOptionEmpty) {
  auto domains = DomainOption::FromString("", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, ParseDomainOptionIncludeSimple) {
  auto domains = DomainOption::FromString("domain.org", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, ParseDomainOptionExcludeSimple) {
  auto domains = DomainOption::FromString("~domain.org", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_THAT(domains.GetExcludeDomains(), UnorderedElementsAre("domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionMultiple) {
  auto domains = DomainOption::FromString(
      "domain.org,~exclude-domain.org,test.domain.org,~test.exclude-domain."
      "org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude-domain.org", "exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionCaseInsensitive) {
  auto domains = DomainOption::FromString(
      "DoMain.org,~excLude-doMain.org,test.domain.org,~tESt.exclude-domain."
      "org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude-domain.org", "exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionMultipleWithWs) {
  auto domains = DomainOption::FromString(
      "domain.org ,  ~exclude-domain.org, test.domain.org "
      ",~test.exclude-domain.org ",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude-domain.org", "exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionIncludeMultiple) {
  auto domains = DomainOption::FromString("domain.org,test.domain.org", ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("domain.org", "test.domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, ParseDomainOptionExcludeMultiple) {
  auto domains = DomainOption::FromString(
      "~exclude-domain.org,~test.exclude-domain.org", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("exclude-domain.org", "test.exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, RemoveDomainsWithNoSubdomain) {
  auto domains = DomainOption::FromString(
      "org,include-domain.org,~com,~exclude-domain.org", ",");
  domains.RemoveDomainsWithNoSubdomain();
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("include-domain.org"));
  EXPECT_THAT(domains.GetExcludeDomains(),
              UnorderedElementsAre("exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, RemoveDomainsWithNoSubdomainButLocalhost) {
  auto domains = DomainOption::FromString(
      "localhost,include-domain.org,~localhost,~exclude-domain.org", ",");
  domains.RemoveDomainsWithNoSubdomain();
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("include-domain.org", "localhost"));
  EXPECT_THAT(domains.GetExcludeDomains(),
              UnorderedElementsAre("exclude-domain.org", "localhost"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionWithDuplications) {
  auto domains = DomainOption::FromString(
      "domain.org,~exclude-domain.org,test.domain.org,~test.exclude-domain.org,"
      "domain.org,~exclude-domain.org,test.domain.org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude-domain.org", "exclude-domain.org"));
}

TEST(AdblockDomainOptionTest, TrailingDot) {
  auto domains = DomainOption::FromString("domain.org.", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, TrailingWildcard) {
  auto domains = DomainOption::FromString("domain.org*", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, TrailingDotAndWildcard) {
  auto domains = DomainOption::FromString("domain.org.*", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("domain.org."));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, TrailingWildcardAndDot) {
  auto domains = DomainOption::FromString("domain.org*.", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, SlashAtTheEnd) {
  auto domains = DomainOption::FromString("domain.org/", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, CaretAtTheEnd) {
  auto domains = DomainOption::FromString("domain.org^", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, InvalidDomainCharacters) {
  auto domains =
      DomainOption::FromString("dom_ain.org,ex@mple.org,test.org", ",");
  EXPECT_THAT(domains.GetIncludeDomains(), UnorderedElementsAre("test.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, DoubleDots) {
  auto domains = DomainOption::FromString("domain..org", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, DoubleComma) {
  {
    auto domains = DomainOption::FromString("domain.org,,example.org", ",");
    EXPECT_THAT(domains.GetIncludeDomains(),
                UnorderedElementsAre("domain.org", "example.org"));
    EXPECT_TRUE(domains.GetExcludeDomains().empty());
  }
  {
    auto domains = DomainOption::FromString("domain.org,,", ",");
    EXPECT_THAT(domains.GetIncludeDomains(),
                UnorderedElementsAre("domain.org"));
    EXPECT_TRUE(domains.GetExcludeDomains().empty());
  }
  {
    auto domains = DomainOption::FromString(",,domain.org", ",");
    EXPECT_THAT(domains.GetIncludeDomains(),
                UnorderedElementsAre("domain.org"));
    EXPECT_TRUE(domains.GetExcludeDomains().empty());
  }
}

TEST(AdblockDomainOptionTest, UrlInsteadDomain) {
  auto domains = DomainOption::FromString("https://example.com", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, SameDomainIncludedAndExcluded) {
  auto domains = DomainOption::FromString(
      "some-domain.org,some-domain.org,~some-domain.org,~some-domain.org", ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("some-domain.org"));
  EXPECT_THAT(domains.GetExcludeDomains(),
              UnorderedElementsAre("some-domain.org"));
}

}  // namespace adblock
