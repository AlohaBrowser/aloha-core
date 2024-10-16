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

#include "components/adblock/core/subscription/subscription_config.h"

#include <map>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "net/base/url_util.h"

namespace {
int g_port_for_testing = 0;

std::string GetHost() {
  GURL url("https://easylist-downloads.adblockplus.org");
  if (!g_port_for_testing) {
    return url.spec();
  }
  GURL::Replacements replacements;
  const std::string port_str = base::NumberToString(g_port_for_testing);
  replacements.SetPortStr(port_str);
  return url.ReplaceComponents(replacements).spec();
}
}  // namespace

namespace adblock {

const GURL& AdblockBaseFilterListUrl() {
  static GURL kAdblockBaseFilterListUrl(GetHost());
  return kAdblockBaseFilterListUrl;
}

const GURL& AcceptableAdsUrl() {
  static GURL kAcceptableAds(GetHost() + "exceptionrules.txt");
  return kAcceptableAds;
}

const GURL& AntiCVUrl() {
  static GURL kAntiCV(GetHost() + "abp-filters-anti-cv.txt");
  return kAntiCV;
}

const GURL& DefaultSubscriptionUrl() {
  static GURL kEasylistUrl(GetHost() + "easylist.txt");
  return kEasylistUrl;
}

const GURL& RecommendedSubscriptionListUrl() {
  static GURL kRecommendedSubscriptionListUrl(GetHost() +
                                              "recommendations.json");
  return kRecommendedSubscriptionListUrl;
}

KnownSubscriptionInfo::KnownSubscriptionInfo() = default;
KnownSubscriptionInfo::~KnownSubscriptionInfo() = default;
KnownSubscriptionInfo::KnownSubscriptionInfo(const KnownSubscriptionInfo&) =
    default;
KnownSubscriptionInfo::KnownSubscriptionInfo(KnownSubscriptionInfo&&) = default;
KnownSubscriptionInfo& KnownSubscriptionInfo::operator=(
    const KnownSubscriptionInfo&) = default;
KnownSubscriptionInfo& KnownSubscriptionInfo::operator=(
    KnownSubscriptionInfo&&) = default;

KnownSubscriptionInfo::KnownSubscriptionInfo(
    GURL url_param,
    std::string title_param,
    std::vector<std::string> languages_param,
    SubscriptionUiVisibility ui_visibility_param,
    SubscriptionFirstRunBehavior first_run_param,
    SubscriptionPrivilegedFilterStatus privileged_status_param)
    : url(url_param),
      title(title_param),
      languages(languages_param),
      ui_visibility(ui_visibility_param),
      first_run(first_run_param),
      privileged_status(privileged_status_param) {}

const std::vector<KnownSubscriptionInfo>& config::GetKnownSubscriptions() {
  // The current list of subscriptions can be downloaded from:
  // https://gitlab.com/eyeo/filterlists/subscriptionlist/-/archive/master/subscriptionlist-master.tar.gz
  // The archive contains files with subscription definitions. The ones we're
  // interested in are ones that declare a [recommendation] keyword in either
  // the "list" or "variant" key.
  // Here's a script that parses the archive into a subscriptions.json:
  // https://gitlab.com/eyeo/adblockplus/abc/adblockpluscore/-/blob/next/build/updateSubscriptions.js
  // The list isn't updated very often. If it starts to become a burden to
  // align the C++ representation, better to update it manually because it also
  // contains visibility and first run behavior options.
  static std::vector<KnownSubscriptionInfo> recommendations = {
      {DefaultSubscriptionUrl(),
       "EasyList",
       {"en"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Subscribe,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "abpindo.txt"),
       "ABPindo",
       {"id", "ms"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "abpvn.txt"),
       "ABPVN List",
       {"vi"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "bulgarian_list.txt"),
       "Bulgarian list",
       {"bg"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "dandelion_sprouts_nordic_filters.txt"),
       "Dandelion Sprout's Nordic Filters",
       {"no", "nb", "nn", "da", "is", "fo", "kl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistchina.txt"),
       "EasyList China",
       {"zh"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistczechslovak.txt"),
       "EasyList Czech and Slovak",
       {"cs", "sk"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistdutch.txt"),
       "EasyList Dutch",
       {"nl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistgermany.txt"),
       "EasyList Germany",
       {"de"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "israellist.txt"),
       "EasyList Hebrew",
       {"he"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "hufilter.txt"),
       "EasyList Hungarian",
       {"hu"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistitaly.txt"),
       "EasyList Italy",
       {"it"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistlithuania.txt"),
       "EasyList Lithuania",
       {"lt"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistpolish.txt"),
       "EasyList Polish",
       {"pl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistportuguese.txt"),
       "EasyList Portuguese",
       {"pt"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistspanish.txt"),
       "EasyList Spanish",
       {"es"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "global-filters.txt"),
       "Global Filters",
       {"th", "el", "sl", "hr", "sr", "bs", "fil"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "indianlist.txt"),
       "IndianList",
       {"bn", "gu", "hi", "pa", "as", "mr", "ml", "te", "kn", "or", "ne", "si",
        "ta", "mai"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "japanese-filters.txt"),
       "Japanese Filters",
       {"ja"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "koreanlist.txt"),
       "KoreanList",
       {"ko"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "latvianlist.txt"),
       "Latvian List",
       {"lv"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "liste_ar.txt"),
       "Liste AR",
       {"ar"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "liste_fr.txt"),
       "Liste FR",
       {"ar", "fr"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "rolist.txt"),
       "ROList",
       {"ro"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "ruadlist.txt"),
       "RuAdList",
       {"ru", "uk", "uz", "kk"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "turkish-filters.txt"),
       "Turkish Filters",
       {"tr"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {AcceptableAdsUrl(),
       "Acceptable Ads",
       {},
       SubscriptionUiVisibility::Invisible,
       SubscriptionFirstRunBehavior::Subscribe,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {AntiCVUrl(),
       "ABP filters",
       {},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Subscribe,
       SubscriptionPrivilegedFilterStatus::Allowed},
      {GURL(GetHost() + "i_dont_care_about_cookies.txt"),
       "I don't care about cookies",
       {},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + ""
                        "fanboy-notifications.txt"),
       "Fanboy's Notifications Blocking List",
       {},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easyprivacy.txt"),
       "EasyPrivacy",
       {},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "fanboy-social.txt"),
       "Fanboy's Social Blocking List",
       {},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {CustomFiltersUrl(),
       "User filters",
       {},
       SubscriptionUiVisibility::Invisible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Allowed},
      {TestPagesSubscriptionUrl(),
       "ABP Test filters",
       {},
       SubscriptionUiVisibility::Invisible,
       SubscriptionFirstRunBehavior::Ignore,
       SubscriptionPrivilegedFilterStatus::Allowed}

      // You can customize subscriptions available on first run and in settings
      // here. Items are displayed in settings in order declared here. See
      // components/adblock/docs/integration-how-to.md, section 'How to change
      // the default filter lists?'. For example:

      // clang-format off
      /*
      {"https://domain.com/subscription.txt",        // URL
       "My Custom Filters",                          // Display name for settings
       {},                                           // Supported languages list, considered for
                                                     // SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch
       SubscriptionUiVisibility::Visible,            // Should the app show a subscription in the settings
       SubscriptionFirstRunBehavior::Subscribe,      // Should the app subscribe on first run
       SubscriptionPrivilegedFilterStatus::Forbidden // Allow or forbid snippets and header filters
      },
      */
      // clang-format on

  };

  return recommendations;
}

bool config::AllowPrivilegedFilters(const GURL& url) {
#if !defined(NDEBUG)
  // Enable priviliged filters from locally hosted filter lists in debug builds.
  // e.g. locally delployed testpages.
  if (net::IsLocalhost(url)) {
    return true;
  }
#endif

  for (const auto& cur : GetKnownSubscriptions()) {
    if (cur.url == url) {
      return cur.privileged_status ==
             SubscriptionPrivilegedFilterStatus::Allowed;
    }
  }

  return false;
}

const std::vector<PreloadedSubscriptionInfo>&
config::GetPreloadedSubscriptionConfiguration() {
  static const std::vector<PreloadedSubscriptionInfo> preloaded_subscriptions =
      {{"*easylist.txt", IDR_ADBLOCK_FLATBUFFER_EASYLIST},
       {"*exceptionrules.txt", IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES},
       {"*abp-filters-anti-cv.txt", IDR_ADBLOCK_FLATBUFFER_ANTICV}};
  return preloaded_subscriptions;
}

void SetFilterListServerPortForTesting(int port_for_testing) {
  g_port_for_testing = port_for_testing;
}

const std::vector<std::string_view>& config::MaybeSplitCombinedAdblockList(
    const GURL& filter_list) {
  static std::vector<std::string_view> EMPTY_VALUE;
  static std::string_view VALUE_EASYLIST_EN = "easylist.txt";
  static std::map<std::string_view, std::vector<std::string_view>>
      filter_lists_map = {
          {"abpindo+easylist.txt", {"abpindo.txt", VALUE_EASYLIST_EN}},
          {"abpvn+easylist", {"abpvn.txt", VALUE_EASYLIST_EN}},
          {"bulgarian_list+easylist.",
           {"bulgarian_list.txt", VALUE_EASYLIST_EN}},
          {"dandelion_sprouts_nordic_filters+easylist.txt",
           {"dandelion_sprouts_nordic_filters.txt", VALUE_EASYLIST_EN}},
          {"easylistchina+easylist.txt",
           {"easylistchina.txt", VALUE_EASYLIST_EN}},
          {"easylistczechslovak+easylist.txt",
           {"easylistczechslovak.txt", VALUE_EASYLIST_EN}},
          {"easylistdutch+easylist.txt",
           {"easylistdutch.txt", VALUE_EASYLIST_EN}},
          {"easylistgermany+easylist.txt",
           {"easylistgermany.txt", VALUE_EASYLIST_EN}},
          {"israellist+easylist.txt", {"israellist.txt", VALUE_EASYLIST_EN}},
          {"hufilter+easylist.txt", {"hufilter.txt", VALUE_EASYLIST_EN}},
          {"easylistitaly+easylist.txt",
           {"easylistitaly.txt", VALUE_EASYLIST_EN}},
          {"easylistlithuania+easylist.txt",
           {"easylistlithuania.txt", VALUE_EASYLIST_EN}},
          {"easylistpolish+easylist.txt",
           {"easylistpolish.txt", VALUE_EASYLIST_EN}},
          {"easylistportuguese+easylist.txt",
           {"easylistportuguese.txt", VALUE_EASYLIST_EN}},
          {"easylistspanish+easylist.txt",
           {"easylistspanish.txt", VALUE_EASYLIST_EN}},
          {"global-filters+easylist.txt",
           {"global-filters.txt", VALUE_EASYLIST_EN}},
          {"indianlist+easylist.txt", {"indianlist.txt", VALUE_EASYLIST_EN}},
          {"japanese+easylist.txt", {"japanese.txt", VALUE_EASYLIST_EN}},
          {"koreanlist+easylist.txt", {"koreanlist.txt", VALUE_EASYLIST_EN}},
          {"latvianlist+easylist.txt", {"latvianlist.txt", VALUE_EASYLIST_EN}},
          {"liste_ar+liste_fr+easylist.txt",
           {"liste_ar.txt", "liste_fr.txt", VALUE_EASYLIST_EN}},
          {"liste_fr+easylist.txt", {"liste_fr.txt", VALUE_EASYLIST_EN}},
          {"rolist+easylist.txt", {"rolist.txt", VALUE_EASYLIST_EN}},
          {"ruadlist+easylist.txt", {"ruadlist.txt", VALUE_EASYLIST_EN}},
          {"turkish+easylist.txt", {"turkish.txt", VALUE_EASYLIST_EN}},
      };
  if (filter_list.host() != AdblockBaseFilterListUrl().host()) {
    // This method works only for ad block maintained lists.
    return EMPTY_VALUE;
  }
  auto path =
      base::TrimString(filter_list.path_piece(), "/", base::TRIM_LEADING);
  if (filter_lists_map.find(path) == filter_lists_map.end()) {
    return EMPTY_VALUE;
  }
  return filter_lists_map[path];
}

}  // namespace adblock
