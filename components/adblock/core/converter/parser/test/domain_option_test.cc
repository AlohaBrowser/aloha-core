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
      "domain.org,~exclude_domain.org,test.domain.org,~test.exclude_domain."
      "org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude_domain.org", "exclude_domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionCaseInsensitive) {
  auto domains = DomainOption::FromString(
      "DoMain.org,~excLude_doMain.org,test.domain.org,~tESt.exclude_domain."
      "org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude_domain.org", "exclude_domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionMultipleWithWs) {
  auto domains = DomainOption::FromString(
      "domain.org ,  ~exclude_domain.org, test.domain.org "
      ",~test.exclude_domain.org ",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude_domain.org", "exclude_domain.org"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionIncludeMultiple) {
  auto domains = DomainOption::FromString("domain.org,test.domain.org", ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("domain.org", "test.domain.org"));
  EXPECT_TRUE(domains.GetExcludeDomains().empty());
}

TEST(AdblockDomainOptionTest, ParseDomainOptionExcludeMultiple) {
  auto domains = DomainOption::FromString(
      "~exclude_domain.org,~test.exclude_domain.org", ",");
  EXPECT_TRUE(domains.GetIncludeDomains().empty());
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("exclude_domain.org", "test.exclude_domain.org"));
}

TEST(AdblockDomainOptionTest, RemoveDomainsWithNoSubdomain) {
  auto domains = DomainOption::FromString(
      "org,include_domain.org,~com,~exclude_domain.org", ",");
  domains.RemoveDomainsWithNoSubdomain();
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("include_domain.org"));
  EXPECT_THAT(domains.GetExcludeDomains(),
              UnorderedElementsAre("exclude_domain.org"));
}

TEST(AdblockDomainOptionTest, RemoveDomainsWithNoSubdomainButLocalhost) {
  auto domains = DomainOption::FromString(
      "localhost,include_domain.org,~localhost,~exclude_domain.org", ",");
  domains.RemoveDomainsWithNoSubdomain();
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("include_domain.org", "localhost"));
  EXPECT_THAT(domains.GetExcludeDomains(),
              UnorderedElementsAre("exclude_domain.org", "localhost"));
}

TEST(AdblockDomainOptionTest, ParseDomainOptionWithDuplications) {
  auto domains = DomainOption::FromString(
      "domain.org,~exclude_domain.org,test.domain.org,~test.exclude_domain.org,"
      "domain.org,~exclude_domain.org,test.domain.org",
      ",");
  EXPECT_THAT(domains.GetIncludeDomains(),
              UnorderedElementsAre("test.domain.org", "domain.org"));
  EXPECT_THAT(
      domains.GetExcludeDomains(),
      UnorderedElementsAre("test.exclude_domain.org", "exclude_domain.org"));
}

}  // namespace adblock
