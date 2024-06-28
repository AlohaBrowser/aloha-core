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
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {
namespace {

std::string RandomAsciiString(size_t length) {
  std::string result(length, ' ');
  for (auto& c : result) {
    c = base::RandInt('a', 'z');
  }
  return result;
}
}  // namespace

class AdblockInstalledSubscriptionImplUrlTest
    : public AdblockInstalledSubscriptionImplTestBase {};

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_NoRules) {
  auto subscription = ConvertAndLoadRules("");
  EXPECT_FALSE(subscription->HasUrlFilter(GURL("https://untracked.com/file"),
                                          "domain.com", ContentType::Stylesheet,
                                          SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Stylesheet) {
  auto subscription = ConvertAndLoadRules(R"(
    abptestpages.org/testfiles/stylesheet/$stylesheet
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/stylesheet/file.css"),
      "domain.com", ContentType::Stylesheet, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Image) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/image/static/$image
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_HandlesLookaroundUrlBlockingFiltersNegativeLookahead) {
  // The filter below employs negative lookahead
  // it should match any url except the ones that are within
  // the (?!) group.
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club|api\.kinogram\.best\/embed\/|cdn\.jsdelivr\.net\/npm\/venom-player)/$third-party,xmlhttprequest,domain=kindkino.ru
    )");

  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://something.streamvid.club"), "kindkino.ru",
      ContentType::Xmlhttprequest, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://foo.org"), "kindkino.ru",
                                         ContentType::Xmlhttprequest, SiteKey(),
                                         FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_HandlesLookaroundUrlBlockingFiltersCaseSensetive) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club/case)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
    )");

  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://something.streamvid.club/case"), "kindkino.ru",
      ContentType::Xmlhttprequest, SiteKey(), FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://something.streamvid.club/CASE"), "kindkino.ru",
      ContentType::Xmlhttprequest, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_NotAffectedByRegexLookaroundFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/image/static/$image
    /^https?://(?![^\s]+\.streamvid\.club)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_NotAffectedByRegexLookaroundFilter_2) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
    /^https?://(?![^\s]+\.streamvid\.club)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_NotAffectedByInvalidRegexFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/image/static/$image
    /[/
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_TrailingSeparator) {
  auto subscription = ConvertAndLoadRules(R"(
    ^pid=Ads^
  )");

  const std::string url = R"(https://c.contentsquare.net/pageview?pid=905&)";

  EXPECT_FALSE(subscription->HasUrlFilter(GURL(url), "domain.com",
                                          ContentType::Image, SiteKey(),
                                          FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_NotAffectedByInvalidRegexFilter_3) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
    /[/
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_LookaroundRegexEnginesMaxUrlSizeStressTest) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club|api\.kinogram\.best\/embed\/|cdn\.jsdelivr\.net\/npm\/venom-player)/$third-party,xmlhttprequest,domain=kindkino.ru
    )");

  std::string url = "https://something.streamvid.club/";
  url.append(RandomAsciiString(url::kMaxURLChars - url.size()));
  const GURL big_url(url);

  EXPECT_EQ(big_url.spec().size(), url::kMaxURLChars);
  EXPECT_FALSE(subscription->HasUrlFilter(big_url, "kindkino.ru",
                                          ContentType::Xmlhttprequest,
                                          SiteKey(), FilterCategory::Blocking));

  url.append(RandomAsciiString(url::kMaxURLChars));
  const GURL bigger_url(url);

  EXPECT_EQ(bigger_url.spec().size(), url::kMaxURLChars * 2);
  EXPECT_FALSE(subscription->HasUrlFilter(bigger_url, "kindkino.ru",
                                          ContentType::Xmlhttprequest,
                                          SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_AcceptsNonNormalizedUrl) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/ad
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https:abptestpages.org/ad:80"),
                                         "domain.com", ContentType::Stylesheet,
                                         SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Script) {
  auto subscription = ConvertAndLoadRules(R"(
    abptestpages.org/testfiles/script/$script
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Subdocument) {
  auto subscription = ConvertAndLoadRules(R"(
    abptestpages.org/testfiles/subdocument/$subdocument
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/subdocument/index.html"),
      "domain.com", ContentType::Subdocument, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_WebRTC) {
  auto subscription = ConvertAndLoadRules(R"(
    $webrtc,domain=abptestpages.org
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/webrtc"), "abptestpages.org",
      ContentType::Webrtc, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Wildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/blocking/wildcard/*/wildcard.png
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/blocking/wildcard/path/component/"
           "wildcard.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_FullPath) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/blocking/full-path.png
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/blocking/full-path.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_PartialPath) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/blocking/partial-path/
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/blocking/partial-path/content.png"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_EndWithCaret) {
  auto subscription = ConvertAndLoadRules(R"(
    ads.example.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://ads.example.com.ua"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainWildcardMiddle1) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a.*.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.c.b.com"), "https://abptestpages.org", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainWildcardMiddle2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a*.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.c.b.com"), "https://abptestpages.org", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainWildcardMiddle3) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a.*b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://abptestpages.org", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_WildcardDomainAndPath1) {
  auto subscription = ConvertAndLoadRules(R"(
    ||d*in.com/*/blocking
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/blocking/wildcard.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_WildcardDomainAndPath2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a*.b.com/*.png
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com/a.png"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://ac.b.com/image.png"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_DomainWildcardStart) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*a.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://abptestpages.org", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://domena.b.com/path"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainWildcardStartEndCaret) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*a.b.com^
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://abptestpages.org", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://domena.b.com/path"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainCaretWildcardEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com^*/path.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://connect.example.com/en_US/path.js"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_CaretEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com^
    @@||example.com/ddm^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://ad.example.com/ddm/ad/infytghiuf/nmys/;ord=1596077603231?"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://ad.example.com/ddm/ad/infytghiuf/nmys/;ord=1596077603231?"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainInParamsNoMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    ||domain.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://example.com/path?domain=https://www.domain.com"),
      "https://abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_SchemeDomainDot) {
  auto subscription = ConvertAndLoadRules(R"(
    ://ads.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://ads.example.com/u?dp=1"), "https://abptestpages.org",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_PathWildcards) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com/a/*/c/script.*.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/a/b/c/script.file.js"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_MultipleCaretAndWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    @@^path1/path2/*/path4/file*.js^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path1/path2/path3/path4/file1.2.3.js?v=1"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_PartdomainNoMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    ||art-domain.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://sub.part-domain.com/path"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_DoubleSlash) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com*/script.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com//script.js"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_Regex) {
  auto subscription = ConvertAndLoadRules(R"(
   @@/^https:\/\/www\.domain(?:\.[a-z]{2,3}){1,2}\/afs\//
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://www.domain.com/afs/iframe.html"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainMatchFilterWithoutDomain1) {
  auto subscription = ConvertAndLoadRules(R"(
   @@||*file_name.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file_name.gif"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainMatchFilterWithoutDomain2) {
  auto subscription = ConvertAndLoadRules(R"(
   @@||*/file_name.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file_name.gif"),
      "https://abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_DomainStart) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.co
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://example.com"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_StartMatchCompleteUrl) {
  auto subscription = ConvertAndLoadRules(R"(
    |https://domain.com/path/file.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/path/file.gif"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainMatchDotWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file.gif"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_DomainWithPort) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com:8888/js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://wwww.example.com:8888/js"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_IPWithPortAndWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    ||1.2.3.4:8060/*/
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://1.2.3.4:8060/path"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://1.2.3.4:8060/path/file.js"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_DomainWithPortAndCaret) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com:8862^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com:8862"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com:8862/path"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://example.com:886"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_SinglePipeCaret) {
  auto subscription = ConvertAndLoadRules(R"(
    |http://example.com^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://example.com:8000/"), "https://abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlAllowListDocument) {
  auto subscription = ConvertAndLoadRules(R"(
    @@^upapi=true$document,domain=thedirect.com|domain2.com
  )");
  EXPECT_FALSE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://backend.upapi.net/pv?upapi=true"), "domain.com",
      SiteKey()));

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://backend.upapi.net/pv?upapi=true"), "thedirect.com",
      SiteKey()));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_DomainSpecific) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=abptestpages.org
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
  // Does not match the same url embedded in a different domain - the rule is
  // domain-specific.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "different.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_DomainSpecificIgnoresWWW) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=abptestpages.org
  )");

  // The filter's domain is abptestpages.org and the request's domain
  // is www.abptestpages.org. The "www" prefix is ignored, the domains
  // are considered to match.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "www.abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_DomainSpecificIsCaseInsensitive) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=abptestpages.org
  )");

  // The filter's domain is abptestpages.org and the request's domain
  // is abptestpages.org. The domains are considered to match.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Other) {
  auto subscription = ConvertAndLoadRules(R"(
    $other,domain=abptestpages.org
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://whatever.com/script.js"),
                                         "abptestpages.org", ContentType::Other,
                                         SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Ping) {
  auto subscription = ConvertAndLoadRules(R"(
    abptestpages.org/*^$ping
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://abptestpages.org/ping"),
                                         "abptestpages.org", ContentType::Ping,
                                         SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_XmlHttpRequest) {
  auto subscription = ConvertAndLoadRules(R"(
    abptestpages.org/testfiles/xmlhttprequest/$xmlhttprequest
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/xmlhttprequest/"
           "request.xml"),
      "abptestpages.org", ContentType::Xmlhttprequest, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_ThirdParty) {
  auto subscription = ConvertAndLoadRules(R"(
    adblockplus-icon-colour-web.svg$third-party
  )");

  // $third-party means the rule applies if the domain is different than the
  // domain of the URL (actually, a bit more complicated than that)
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/adblockplus-icon-colour-web.svg"),
      "google.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));

  // Does not apply on same domain.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/adblockplus-icon-colour-web.svg"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_ThirdParty_2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||adgrx.com^$third-party
    @@||fls.doubleclick.net^$subdocument,image
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://rtb.adgrx.com/segments/"
           "EEXrM7_yY0aoq_OtZxYHORtz55uneYk8VAiLioGMm14=/47510.gif"),
      "8879538.fls.doubleclick.net", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));

  EXPECT_FALSE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://rtb.adgrx.com/segments/"
           "EEXrM7_yY0aoq_OtZxYHORtz55uneYk8VAiLioGMm14=/47510.gif"),
      "8879538.fls.doubleclick.net", SiteKey()));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_RegexEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||popin.cc/popin_discovery
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://api.popin.cc/popin_discovery5-min.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_NotThirdParty) {
  auto subscription = ConvertAndLoadRules(R"(
    abb-logo.png$~third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/abb-logo.png"), "abptestpages.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  // Does not apply on different domain.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/abb-logo.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterBlocking_Popup) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/popup/link.html^$popup
  )");

  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", SiteKey(), FilterCategory::Blocking));

  // No allowing filter:
  EXPECT_FALSE(subscription->HasPopupFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", SiteKey(), FilterCategory::Allowing));

  // Does not match if the content type is different.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", ContentType::Subdocument, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_MultipleTypes) {
  auto subscription = ConvertAndLoadRules(R"(
    ||abptestpages.org/testfiles/popup/link.html^$popup,image,script
  )");

  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", SiteKey(), FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/popup/link.html"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_UppercaseFilterAndURL) {
  // A filter with uppercase component should be matched by a uppercase URL,
  // this requires keywords and urls to be converted to lowercase during filter
  // parsing.
  auto subscription = ConvertAndLoadRules(R"(
    ||yahoo.com/bidRequest?
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://c2shb.ssp.yahoo.com/"
           "bidRequest?dcn=8a9691510171713aaede3c85d0ab0026&pos=desktop_sky_"
           "right_bottom&cmd=bid&secure=1&gdpr=1&euconsent="
           "BO3TikKO3TikKAbABBENC6AAAAAtmAAA"),
      "www.dailymail.co.uk", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_UppercaseDefinition) {
  // A filter with uppercase component should be matched by a lowercase URL,
  // this requires keywords to be converted to lowercase during filter parsing.
  auto subscription = ConvertAndLoadRules(R"(
    ||bttrack.com/Pixel/$image,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://bttrack.com/pixel/"
           "cookiesync?source=14b8c562-d12b-418b-b680-ad517d5839ec"),
      "super.abril.com.br", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_UppercaseURL) {
  // A filter with lowercase component should be matched by an uppercase URL,
  // this requires keywords to be converted to lowercase during URL matching.
  auto subscription = ConvertAndLoadRules(R"(
    ||bttrack.com/pixel/$image,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://bttrack.com/Pixel/"
           "cookiesync?source=14b8c562-d12b-418b-b680-ad517d5839ec"),
      "super.abril.com.br", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilter_CaseSensitiveMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/match-case/static/*/abc.png$match-case
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/match-case/static/path/abc.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
  // Does not match if the case is different
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/match-case/static/path/ABC.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_AllowlistThirdPartyFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||amazon-adsystem.com^$third-party
    @@||amazon-adsystem.com//ecm
    @@||amazon-adsystem.com/ecm
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://s.amazon-adsystem.com//"
           "ecm3?ex=visualiq&id=a78f7b3f-a525-45eb-a1ed-5eb5eab339af"),
      "www.wwe.com", ContentType::Image, SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilter_IgnoresInvalidFilterOption) {
  auto subscription = ConvertAndLoadRules(R"(
    @@||google.com/recaptcha/$csp,subdocument
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL(" https://www.google.com/recaptcha/api2/a"), "vidoza.net",
      ContentType::Subdocument, SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Sitekey) {
  auto subscription = ConvertAndLoadRules(R"(
    @@$document,sitekey=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANGtTstne7e8MbmDHDiMFkGbcuBgXmiVesGOG3gtYeM1EkrzVhBjGUvKXYE4GLFwqty3v5MuWWbvItUWBTYoVVsCAwEAAQ
  )");

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Document, GURL("https://whatever.com"), "whatever.com",
      SiteKey("MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANGtTstne7e8MbmDHDiMFkGbcuBgXmiV"
              "esGOG3gt"
              "YeM1EkrzVhBjGUvKXYE4GLFwqty3v5MuWWbvItUWBTYoVVsCAwEAAQ")));
  // No allow rule without sitekey
  EXPECT_FALSE(subscription->HasSpecialFilter(SpecialFilterType::Document,
                                              GURL("https://whatever.com"),
                                              "whatever.com", SiteKey()));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Popup) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/popup
    ||abptestpages.org/testfiles/popup_exception/link.html^$popup
    @@||abptestpages.org/testfiles/popup_exception/link.html^$popup
  )");

  // Finds the blocking filter:
  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://abptestpages.org/testfiles/popup_exception/"
           "link.html"),
      "abptestpages.org", SiteKey(), FilterCategory::Blocking));

  // But also finds the allowing filter:
  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://abptestpages.org/testfiles/popup_exception/"
           "link.html"),
      "abptestpages.org", SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterException_XmlHttpRequest) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/xmlhttprequest
    ||abptestpages.org/testfiles/xmlhttprequest_exception/*
    @@abptestpages.org/testfiles/xmlhttprequest_exception/$xmlhttprequest
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/"
           "xmlhttprequest_exception/link.html"),
      "https://abptestpages.org", ContentType::Xmlhttprequest, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Ping) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/ping
    abptestpages.org/*^$ping
    @@abptestpages.org/en/exceptions/ping*^$ping
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/en/exceptions/ping/link.html"),
      "https://abptestpages.org", ContentType::Ping, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterException_Subdocument) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/subdocument
    ||abptestpages.org/testfiles/subdocument_exception/*
    @@abptestpages.org/testfiles/subdocument_exception/$subdocument
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/subdocument_exception/"
           "link.html"),
      "abptestpages.org", ContentType::Subdocument, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Script) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/script
    ||abptestpages.org/testfiles/script_exception/*
    @@abptestpages.org/testfiles/script_exception/$script
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script_exception/"
           "link.html"),
      "abptestpages.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Image) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/image
    ||abptestpages.org/testfiles/image_exception/*
    @@abptestpages.org/testfiles/image_exception/$image
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/image_exception/"
           "image.jpg"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_Stylesheet) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/stylesheet
    ||abptestpages.org/testfiles/stylesheet_exception/*
    @@abptestpages.org/testfiles/stylesheet_exception/$stylesheet
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://abptestpages.org/"
                                              "testfiles/stylesheet_exception/"
                                              "style.css"),
                                         "abptestpages.org",
                                         ContentType::Stylesheet, SiteKey(),
                                         FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterException_WebSocket) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/websocket
    $websocket,domain=abptestpages.org
    @@$websocket,domain=abptestpages.org
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.org/socket.wss"), "abptestpages.org",
      ContentType::Websocket, SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlRegexAnythingEndingOnline) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterRegexContains$WithFilterOptions) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/cdn\.[0-9a-z]{3,6}\.xyz\/[a-z0-9]{8,}\.js$/$script,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));

  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterRegexContains$WithoutFilterOption) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/cdn\.[0-9a-z]{3,6}\.xyz\/[a-z0-9]{8,}\.js$/
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

// TODO generic block filters may soon be removed from the ContentType enum and
// handled internally.
TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterException_GenericBlock) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/genericblock
    /testfiles/genericblock/generic.png
    /testfiles/genericblock/specific.png$domain=abptestpages.org
    @@||abptestpages.org/en/exceptions/genericblock$genericblock
  )");

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Genericblock,
      GURL("https://abptestpages.org/en/exceptions/genericblock"),
      "abptestpages.org", SiteKey()));
  // Since there is a genericblock rule for this parent, we would search for
  // specific-only rules
  // The rule /testfiles/genericblock/generic.png does not apply as it is not
  // domain-specific:
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/genericblock/"
           "generic.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::DomainSpecificBlocking));
  // The rule
  // /testfiles/genericblock/specific.png$domain=abptestpages.org
  // applies:
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/genericblock/"
           "specific.png"),
      "abptestpages.org", ContentType::Image, SiteKey(),
      FilterCategory::DomainSpecificBlocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterException_DomainSpecificExclusion) {
  auto subscription = ConvertAndLoadRules(R"(
||media.net^$third-party
@@||media.net^$document
@@||media.net^$third-party,domain=~fandom.com
@@||contextual.media.net/bidexchange.js
  )");

  // This reflects a bug squished IRL:
  // the URL is blocked by ||media.net^$third-party and should NOT be allowed
  // by any of the allow rules because:
  // 1. It's not a DOCUMENT type
  // 2. The third-party allow rule does not apply on roblox.fandom.com
  // 3. bidexchange.js is a path allowed only for a different page
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://hbx.media.net/bidexchange.js?cid=8CUDYP2MO&version=6.1"),
      "roblox.fandom.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://hbx.media.net/bidexchange.js?cid=8CUDYP2MO&version=6.1"),
      "roblox.fandom.com", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, UrlFilterWithHashSign) {
  auto subscription = ConvertAndLoadRules(R"(
    @@||search.twcc.com/#web/$elemhide
  )");

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Elemhide, GURL("https://search.twcc.com/#web/"), "",
      SiteKey()));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterWildcardUrlShortKeywords) {
  auto subscription = ConvertAndLoadRules(R"(
    /test*iles/a/b.png
    /test*iles/a/b.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/a/b.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/a/b.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, RegexFilterNotLowercased) {
  // \D+ matches "not digits", while \d+ matches "digits":
  auto subscription = ConvertAndLoadRules(R"(
    /test\D+.png/
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://domain.com/testabc.png"),
                                         "domain.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));

  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://domain.com/test123.png"), "domain.com", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, PipeInURL) {
  // See: https://github.com/gatling/gatling/issues/1272
  // These | characters in the middle of the filter should match the literal
  // | characters in the URL, not be treated as anchors.
  auto subscription = ConvertAndLoadRules(R"(
    /addyn|*|adtech;
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://adserver.adtech.de/"
           "addyn|3.0|296|3872016|0|16|ADTECH;loc=100;target=_blank;misc="
           "1043156433;rdclick=http://ba.ccm2.net/RealMedia/ads/click_lx.ads/"
           "fr_ccm_hightech/news/L20/1043156433/Position3/OasDefault/"
           "autopromo_keljob_ccm/autopromo_kelformation_1_ccm.html/"
           "574b7276735648616451774141686f70?"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, MultiplePipeCharacters) {
  // This filter combines | characters used as:
  // - host anchor
  // - normal text character
  // - right anchor
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com/abc|def*.jpg|
  )");

  // Correct match.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://subdomain.example.com/abc|def/content.jpg"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));

  // Incorrect, the URL does not end with .jpg, although .jpg occurs in the URL.
  // Right anchor constraint not met.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://subdomain.example.com/abc|def/file.jpg/content"),
      "domain.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));

  // Incorrect, the URL does not start with a subdomain of example.com.
  // Host anchor constraint not met.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://notexample.com/abc|def/content.jpg"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));

  // Incorrect, the URL does not contain the | in the text.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://subdomain.example.com/abc/def/content.jpg"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest, PipeEndsFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    example*|
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("http://example.com/"),
                                         "domain.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));
}

// See DPD-1913
TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       InvalidFilterDoesNotCrashParser) {
  auto subscription = ConvertAndLoadRules(R"(
    ||
    |
  )");
  // Filters are too short and were rejected by parser.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://example.com/content.jpg"), "example.com", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

// See DPD-1978
TEST_F(AdblockInstalledSubscriptionImplUrlTest, DomainSpecificSinglePipe) {
  auto subscription = ConvertAndLoadRules(R"(
    |$domain=example.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("http://example.com/content.jpg"),
                                         "example.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://example.com/content.jpg"), "example.net", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

// See DPD-1978
TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       DomainSpecificSinglePipeWithWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    |*$domain=example.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("http://example.com/content.jpg"),
                                         "example.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://example.com/content.jpg"), "example.net", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

// See DPD-1978
TEST_F(AdblockInstalledSubscriptionImplUrlTest, DomainSpecificDoublePipe) {
  auto subscription = ConvertAndLoadRules(R"(
    ||$domain=example.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("http://example.com/content.jpg"),
                                         "example.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://example.com/content.jpg"), "example.net", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

// See DPD-1978
TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       DomainSpecificDoublePipeWithWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*$domain=example.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("http://example.com/content.jpg"),
                                         "example.com", ContentType::Image,
                                         SiteKey(), FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://example.com/content.jpg"), "example.net", ContentType::Image,
      SiteKey(), FilterCategory::Blocking));
}

TEST(AdblockInstalledSubscriptionImplTest,
     ConvertMoreRegexFiltersThanCacheCapacity) {
  std::vector<std::string> filters;
  // Create a lot of regex filters
  for (int i = 0; i < RegexMatcher::kMaxPrebuiltPatterns; i++) {
    // Match any word followed by the numerical value of i, then another word.
    filters.push_back(base::StringPrintf("/.*word%dword.*/", i));
  }
  // Add one more, this one will not get prebuilt
  filters.push_back(base::StringPrintf("/.*word%dword.*/", 1000));

  auto buffer = FlatbufferConverter::Convert(
      filters, GURL{"http://data.com/filters.txt"}, false);
  ASSERT_TRUE(buffer);
  auto subscription = base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(buffer), Subscription::InstallationState::Installed,
      base::Time());
  // Ensure a URL that matches our "extra" regex filter is matched.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://word1000word.com/ad.jpg"), "example.com",
      ContentType::Image, {}, FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_WildcardDomainMatch) {
  // Using a wildcard specifier in the domain list.
  auto subscription = ConvertAndLoadRules(R"(
    script/$script,domain=example.*
  )");

  // example.com is allowed.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "example.com", ContentType::Script, SiteKey(), FilterCategory::Blocking));
  // example.co.uk is allowed, it's a two-component TLD and the wildcard
  // specifier matches those as well.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "example.co.uk", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  // a subdomain is matched too.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "sub.example.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "sub.example.co.uk", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockInstalledSubscriptionImplUrlTest,
       UrlFilterBlocking_WildcardDomainMismatch) {
  // Using a wildcard specifier in the domain list.
  auto subscription = ConvertAndLoadRules(R"(
    script/$script,domain=example.*
  )");

  // .evil is not a known TLD
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "example.evil", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  // .blogspot.com is a valid TLD but it's a private registrar.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "example.blogspot.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  // notexample.com is a different domain than example.com, should not be
  // matched by example.*.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "notexample.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  // .abc.com is not a TLD at all.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://abptestpages.org/testfiles/script/script.js"),
      "example.abc.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

}  // namespace adblock
