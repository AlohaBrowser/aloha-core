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

#include "chrome/browser/extensions/api/adblock_private/adblock_private_apitest_base.h"

#include "chrome/browser/adblock/adblock_chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/adblock_private.h"
#include "chrome/common/extensions/api/tabs.h"
#include "components/adblock/content/browser/factories/adblock_request_throttle_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/test/adblock_browsertest_base.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/net/adblock_request_throttle.h"
#include "extensions/common/switches.h"

namespace extensions {

void AdblockPrivateApiTestBase::SetUpCommandLine(
    base::CommandLine* command_line) {
  ExtensionApiTest::SetUpCommandLine(command_line);
  AllowTestExtension(command_line);
  if (IsIncognito()) {
    EnableIncognitoMode(command_line);
  }
}

void AdblockPrivateApiTestBase::SetUpOnMainThread() {
  ExtensionApiTest::SetUpOnMainThread();

  // When any of that fails we need to update comment in adblock_private.idl
  ASSERT_EQ(api::tabs::TAB_ID_NONE, -1);
  ASSERT_EQ(SessionID::InvalidValue().id(), -1);

  adblock::SubscriptionServiceFactory::GetForBrowserContext(
      browser()->profile()->GetOriginalProfile())
      ->GetFilteringConfiguration(adblock::kAdblockFilteringConfigurationName)
      ->RemoveCustomFilter(adblock::kAllowlistEverythingFilter);
  // Allow requests for filter lists to be "downloaded" immediately, otherwise
  // the tests will hang for 30 seconds.
  adblock::AdblockRequestThrottleFactory::GetForBrowserContext(
      browser()->profile()->GetOriginalProfile())
      ->AllowRequestsAfter(base::Seconds(0));

  AdblockChromeContentBrowserClient::ForceAdblockProxyForTesting();
}

bool AdblockPrivateApiTestBase::IsIncognito() {
  return false;
}

std::string AdblockPrivateApiTestBase::GetApiEndpoint() {
  return "adblockPrivate";
}

bool AdblockPrivateApiTestBase::RunTest(const std::string& subtest) {
  std::string page_url = "main.html?subtest=" + subtest;
  page_url += "&api=" + GetApiEndpoint();
  return RunExtensionTest("adblock_private",
                          {.extension_url = page_url.c_str()},
                          {.allow_in_incognito = IsIncognito(),
                           .load_as_component = !IsIncognito()});
}

bool AdblockPrivateApiTestBase::RunTestWithParams(
    const std::string& subtest,
    const std::map<std::string, std::string>& params) {
  if (params.empty()) {
    return RunTest(subtest);
  }
  std::string subtest_with_params = subtest;
  for (const auto& [key, value] : params) {
    subtest_with_params += "&" + key + "=" + value;
  }
  return RunTest(subtest_with_params);
}

void AdblockPrivateApiTestBase::AllowTestExtension(
    base::CommandLine* command_line) {
  command_line->AppendSwitchASCII(switches::kAllowlistedExtensionID,
                                  "hfkjbmnbjpodjjpikbpnphphoimfacom");
}

void AdblockPrivateApiTestBase::EnableIncognitoMode(
    base::CommandLine* command_line) {
  command_line->AppendSwitch(::switches::kIncognito);
}

}  // namespace extensions
