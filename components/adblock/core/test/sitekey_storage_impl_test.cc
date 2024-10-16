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

#include "components/adblock/core/sitekey_storage_impl.h"

#include "components/adblock/core/common/adblock_constants.h"
#include "gtest/gtest.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
static constexpr char test_uri[] =
    "/info/"
    "Liquidit%C3%A4t.html?ses="
    "Y3JlPTEzNTUyNDE2OTImdGNpZD13d3cuYWZmaWxpbmV0LXZlcnplaWNobmlzLmRlNTB"
    "jNjAwNzIyNTlkNjQuNDA2MjE2MTImZmtpPTcyOTU2NiZ0YXNrPXNlYXJjaCZkb21haW49Y"
    "WZmaWxpbmV0LXZlcnplaWNobmlzL"
    "mRlJnM9ZGZmM2U5MTEzZGNhMWYyMWEwNDcmbGFuZ3VhZ2U9ZGUmYV9pZD0yJmtleXdvcmQ"
    "9TGlxdWlkaXQlQzMlQTR0JnBvcz0"
    "yJmt3cz03Jmt3c2k9OA==&token=AG06ipCV1LptGtY_"
    "9gFnr0vBTPy4O0YTvwoTCObJ3N3ckrQCFYIA3wod2TwAjxgAIABQv5"
    "WiAlCH8qgOUJGr9g9QmuuEG1CDnK0pUPbRrk5QhqDgkQNxP4Qqhz9xZe4";
static constexpr char test_host[] = "http://www.affilinet-verzeichnis.de";
static constexpr char test_user_agent[] =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.21 (KHTML, like "
    "Gecko) Chrome/25.0.1349.2 Safari/537.21";
static constexpr char test_public_key[] =
    "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANnylWw2vLY4hUn9w06zQKbhKBfvjFUC"
    "sdFlb6TdQhxb9RXWXuI4t31c+o8fYOv/s8q1LGP"
    "ga3DE1L/tHU4LENMCAwEAAQ==";
static constexpr char not_the_right_public_key[] =
    "GSwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANnylWw2vLY4hUn9w06zQKbhKBfvjFUC"
    "sdFos6TdQhxb9RXWXuI4t31c+o8fYOv/s8q1LGP"
    "ga3DE1L/tHU4LENMCAwEAAQ==";
static constexpr char test_signature[] =
    "nLH8Vbc1rzmy0Q+Xg+bvm43IEO42h8rq5D9C0WCn/Y3ykgAoV4npzm7eMlqBSwZBLA/"
    "0DuuVsfTJT9MOVaurcA==";
}  // namespace

namespace adblock {

class AdblockSitekeyStorageFlatbufferTest : public testing::Test {
 public:
  AdblockSitekeyStorageFlatbufferTest() = default;

  void SetUp() override { storage_ = std::make_unique<SitekeyStorageImpl>(); }

  void ProcessResponseHeader(const std::string& public_key,
                             const std::string& signature,
                             const std::string& host,
                             const std::string& uri,
                             const std::string& user_agent) {
    std::string sitekey = public_key + "_" + signature;
    auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
    headers->SetHeader(adblock::kSiteKeyHeaderKey, sitekey);

    GURL url = GURL{host + uri};
    storage_->ProcessResponseHeaders(url, headers, user_agent);
  }

  GURL test_url{std::string(test_host) + std::string(test_uri)};
  std::unique_ptr<SitekeyStorageImpl> storage_;
};

TEST_F(AdblockSitekeyStorageFlatbufferTest, NoUserAgentInfo) {
  ProcessResponseHeader(test_public_key, test_signature, test_host, test_uri,
                        "");
  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, NoSiteKeyHeader) {
  auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("");

  storage_->ProcessResponseHeaders(test_url, headers, test_user_agent);

  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, WrongFormatOfSitekeyHeader) {
  std::string sitekey = std::string(test_public_key) + "NotAnUnderscore" +
                        std::string(test_signature);
  auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  headers->SetHeader(adblock::kSiteKeyHeaderKey, sitekey);

  storage_->ProcessResponseHeaders(test_url, headers, test_user_agent);

  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, InvalidSitekeyPublicKeyNotB64) {
  ProcessResponseHeader("NotAB64EncodedKey", test_signature, test_host,
                        test_uri, test_user_agent);
  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, InvalidSitekeySignatureNotB64) {
  ProcessResponseHeader(test_public_key, "NotAB64EncodedSignature", test_host,
                        test_uri, test_user_agent);
  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, InvalidSitekeyCannotVerify) {
  ProcessResponseHeader(not_the_right_public_key, test_signature, test_host,
                        test_uri, test_user_agent);
  EXPECT_FALSE(storage_->FindSiteKeyForAnyUrl({test_url}).has_value());
}

TEST_F(AdblockSitekeyStorageFlatbufferTest, ValidSignature) {
  ProcessResponseHeader(test_public_key, test_signature, test_host, test_uri,
                        test_user_agent);
  std::pair<GURL, SiteKey> expected{
      GURL{test_url},
      SiteKey{std::string(test_public_key)
                  .substr(0, std::string(test_public_key).find("=="))}};
  EXPECT_EQ(expected, storage_->FindSiteKeyForAnyUrl({test_url}).value());
}

// DPD-1912: This test works with Base64DecodePolicy::kForgiving and fails
// with Base64DecodePolicy::kStrict. Uses data from a real page.
TEST_F(AdblockSitekeyStorageFlatbufferTest,
       SignatureValidWithRelaxedDecodePolicy) {
  constexpr char public_key[] =
      "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJRmzcpTevQqkWn6dJuX/N/"
      "Hxl7YxbOwy8+73ijqYSQEN+WGxrruAKtZtliWC86+ewQ0msW1W8psOFL/b00zWqsCAwEAAQ";
  constexpr char signature[] =
      "kJCR9f/Vb/"
      "NJwTNvLcnLVpM82hW+"
      "6DiMmiBNLSpCBF0LuCTQ0LQZzUaNf5RK8TPaRZKOpTahrMY0B1fCr82MwA";
  constexpr char host[] = "http://skillpanda.com";
  constexpr char uri[] = "/";
  constexpr char user_agent[] =
      "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
      "Chrome/114.0.0.0 Safari/537.36";
  auto url = GURL{std::string(host) + std::string(uri)};
  ProcessResponseHeader(public_key, signature, host, uri, user_agent);
  std::pair<GURL, SiteKey> expected{
      GURL{url}, SiteKey{std::string(public_key)
                             .substr(0, std::string(public_key).find("=="))}};
  EXPECT_EQ(expected, storage_->FindSiteKeyForAnyUrl({url}).value());
}

}  // namespace adblock
