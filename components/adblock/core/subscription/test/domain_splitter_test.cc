
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

#include "components/adblock/core/subscription/domain_splitter.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

TEST(AdblockDomainSplitterTest, SimpleDomain) {
  DomainSplitter splitter("example.com");
  EXPECT_EQ(splitter.FindNextSubdomain(), "example.com");
  EXPECT_EQ(splitter.FindNextSubdomain(), "com");
  // For matching wildcard TLDs:
  EXPECT_EQ(splitter.FindNextSubdomain(), "example.");
  EXPECT_EQ(splitter.FindNextSubdomain(), absl::nullopt);
}

TEST(AdblockDomainSplitterTest, MultiComponentDomain) {
  DomainSplitter splitter("subdomain.example.com");
  EXPECT_EQ(splitter.FindNextSubdomain(), "subdomain.example.com");
  EXPECT_EQ(splitter.FindNextSubdomain(), "example.com");
  EXPECT_EQ(splitter.FindNextSubdomain(), "com");
  // For matching wildcard TLDs:
  EXPECT_EQ(splitter.FindNextSubdomain(), "subdomain.example.");
  EXPECT_EQ(splitter.FindNextSubdomain(), "example.");
  EXPECT_EQ(splitter.FindNextSubdomain(), absl::nullopt);
}

TEST(AdblockDomainSplitterTest, DomainIsRegistrar) {
  DomainSplitter splitter("gov.uk");
  EXPECT_EQ(splitter.FindNextSubdomain(), "gov.uk");
  EXPECT_EQ(splitter.FindNextSubdomain(), "uk");
  // There are no wildcard TLDs to match because gov.uk is a registrar already.
  EXPECT_EQ(splitter.FindNextSubdomain(), absl::nullopt);
}

TEST(AdblockDomainSplitterTest, Localhost) {
  DomainSplitter splitter("localhost");
  EXPECT_EQ(splitter.FindNextSubdomain(), "localhost");
  EXPECT_EQ(splitter.FindNextSubdomain(), absl::nullopt);
}

TEST(AdblockDomainSplitterTest, EmptyDomain) {
  DomainSplitter splitter("");
  EXPECT_EQ(splitter.FindNextSubdomain(), absl::nullopt);
}

}  // namespace adblock
