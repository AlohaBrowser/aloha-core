// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

#include <map>
#include <string>

#include "chrome/browser/extensions/api/adblock_private/adblock_private_apitest_base.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"

namespace extensions {

class AdblockPrivateApiTest
    : public AdblockPrivateApiTestBase,
      public testing::WithParamInterface<
          std::tuple<AdblockPrivateApiTestBase::EyeoExtensionApi,
                     AdblockPrivateApiTestBase::Mode>> {
 public:
  AdblockPrivateApiTest() {}
  ~AdblockPrivateApiTest() override = default;
  AdblockPrivateApiTest(const AdblockPrivateApiTest&) = delete;
  AdblockPrivateApiTest& operator=(const AdblockPrivateApiTest&) = delete;

  bool IsIncognito() override {
    return std::get<1>(GetParam()) ==
           AdblockPrivateApiTestBase::Mode::Incognito;
  }

  std::string GetApiEndpoint() override {
    return std::get<0>(GetParam()) ==
                   AdblockPrivateApiTestBase::EyeoExtensionApi::Old
               ? "adblockPrivate"
               : "eyeoFilteringPrivate";
  }

  std::map<std::string, std::string> FindExpectedDefaultFilterLists() {
    DCHECK(browser()->profile()->GetOriginalProfile());
    auto selected = adblock::SubscriptionServiceFactory::GetForBrowserContext(
                        browser()->profile()->GetOriginalProfile())
                        ->GetFilteringConfiguration(
                            adblock::kAdblockFilteringConfigurationName)
                        ->GetFilterLists();
    const auto easylist = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(), "easylist.txt");
        });
    const auto exceptions = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(),
                                "exceptionrules.txt");
        });
    const auto snippets = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(),
                                "abp-filters-anti-cv.txt");
        });
    if (easylist == selected.end() || exceptions == selected.end() ||
        snippets == selected.end()) {
      return std::map<std::string, std::string>{};
    }
    return std::map<std::string, std::string>{
        {"easylist", easylist->spec()},
        {"exceptions", exceptions->spec()},
        {"snippets", snippets->spec()}};
  }
};

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckEnabled) {
  EXPECT_TRUE(RunTest("setEnabled_isEnabled")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckAAEnabled) {
  EXPECT_TRUE(RunTest(GetApiEndpoint() == "adblockPrivate"
                          ? "setAAEnabled_isAAEnabled"
                          : "setAAEnabled_isAAEnabled_newAPI"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, GetBuiltInSubscriptions) {
  if (GetApiEndpoint() == "adblockPrivate") {
    EXPECT_TRUE(RunTest("getBuiltInSubscriptions")) << message_;
  }
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, InstallSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("installSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, UninstallSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("uninstallSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SubscriptionsManagement) {
  auto params = FindExpectedDefaultFilterLists();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       SubscriptionsManagementConfigDisabled) {
  auto params = FindExpectedDefaultFilterLists();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  params.insert({"disabled", "true"});
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AllowedDomainsManagement) {
  EXPECT_TRUE(RunTest("allowedDomainsManagement")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, CustomFiltersManagement) {
  EXPECT_TRUE(RunTest("customFiltersManagement")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdBlockedEvents) {
  EXPECT_TRUE(RunTest("adBlockedEvents")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdAllowedEvents) {
  EXPECT_TRUE(RunTest("adAllowedEvents")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SessionStats) {
  EXPECT_TRUE(RunTest("sessionStats")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AllowedDomainsEvent) {
  EXPECT_TRUE(RunTest("allowedDomainsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, EnabledStateEvent) {
  EXPECT_TRUE(RunTest("enabledStateEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, FilterListsEvent) {
  EXPECT_TRUE(RunTest("filterListsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, CustomFiltersEvent) {
  EXPECT_TRUE(RunTest("customFiltersEvent")) << message_;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AdblockPrivateApiTest,
    testing::Combine(
        testing::Values(AdblockPrivateApiTestBase::EyeoExtensionApi::Old,
                        AdblockPrivateApiTestBase::EyeoExtensionApi::New),
        testing::Values(AdblockPrivateApiTestBase::Mode::Normal,
                        AdblockPrivateApiTestBase::Mode::Incognito)));

}  // namespace extensions
