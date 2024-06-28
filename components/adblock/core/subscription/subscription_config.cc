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

#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"

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
      {GURL(GetHost() + "abpindo+easylist.txt"),
       "ABPindo+EasyList",
       {"id", "ms"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "abpvn+easylist.txt"),
       "ABPVN List+EasyList",
       {"vi"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "bulgarian_list+easylist.txt"),
       "Bulgarian list+EasyList",
       {"bg"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "dandelion_sprouts_nordic_filters+easylist.txt"),
       "Dandelion Sprout's Nordic Filters+EasyList",
       {"no", "nb", "nn", "da", "is", "fo", "kl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {DefaultSubscriptionUrl(),
       "EasyList",
       {"en"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistchina+easylist.txt"),
       "EasyList China+EasyList",
       {"zh"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistczechslovak+easylist.txt"),
       "EasyList Czech and Slovak+EasyList",
       {"cs", "sk"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistdutch+easylist.txt"),
       "EasyList Dutch+EasyList",
       {"nl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistgermany+easylist.txt"),
       "EasyList Germany+EasyList",
       {"de"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "israellist+easylist.txt"),
       "EasyList Hebrew+EasyList",
       {"he"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "hufilter+easylist.txt"),
       "EasyList Hungarian+EasyList",
       {"hu"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistitaly+easylist.txt"),
       "EasyList Italy+EasyList",
       {"it"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistlithuania+easylist.txt"),
       "EasyList Lithuania+EasyList",
       {"lt"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistpolish+easylist.txt"),
       "EasyList Polish+EasyList",
       {"pl"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistportuguese+easylist.txt"),
       "EasyList Portuguese+EasyList",
       {"pt"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "easylistspanish+easylist.txt"),
       "EasyList Spanish+EasyList",
       {"es"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "global-filters+easylist.txt"),
       "Global Filters+EasyList",
       {"th", "el", "sl", "hr", "sr", "bs"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "indianlist+easylist.txt"),
       "IndianList+EasyList",
       {"bn", "gu", "hi", "pa", "as", "mr", "ml", "te", "kn", "or", "ne", "si"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "japanese-filters+easylist.txt"),
       "Japanese Filters+EasyList",
       {"ja"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "koreanlist+easylist.txt"),
       "KoreanList+EasyList",
       {"ko"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "latvianlist+easylist.txt"),
       "Latvian List+EasyList",
       {"lv"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "liste_ar+liste_fr+easylist.txt"),
       "Liste AR+Liste FR+EasyList",
       {"ar"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "liste_fr+easylist.txt"),
       "Liste FR+EasyList",
       {"fr"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "rolist+easylist.txt"),
       "ROList+EasyList",
       {"ro"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "ruadlist+easylist.txt"),
       "RuAdList+EasyList",
       {"ru", "uk", "uz", "kk"},
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch,
       SubscriptionPrivilegedFilterStatus::Forbidden},
      {GURL(GetHost() + "turkish-filters+easylist.txt"),
       "Turkish Filters+EasyList",
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
       SubscriptionUiVisibility::Visible,
       SubscriptionFirstRunBehavior::Subscribe,
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

}  // namespace adblock
