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

#include "chrome/browser/adblock/adblock_chrome_content_browser_client.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {
enum class Mode { Normal, Incognito };

void AllowTestExtension(base::CommandLine* command_line) {
  command_line->AppendSwitchASCII(extensions::switches::kAllowlistedExtensionID,
                                  "hjlolhdkflcejcekagdgahplimkppjml");
}

void EnableIncognitoMode(base::CommandLine* command_line) {
  command_line->AppendSwitch(::switches::kIncognito);
}
}  // namespace

class EyeoFilteringPrivateApiTest : public ExtensionApiTest,
                                    public testing::WithParamInterface<Mode> {
 public:
  EyeoFilteringPrivateApiTest() {}
  ~EyeoFilteringPrivateApiTest() override = default;
  EyeoFilteringPrivateApiTest(const EyeoFilteringPrivateApiTest&) = delete;
  EyeoFilteringPrivateApiTest& operator=(const EyeoFilteringPrivateApiTest&) =
      delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
    AllowTestExtension(command_line);
    if (IsIncognito()) {
      EnableIncognitoMode(command_line);
    }
  }

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    auto* adblock_configuration =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile())
            ->GetFilteringConfiguration(
                adblock::kAdblockFilteringConfigurationName);
    if (adblock_configuration) {
      adblock_configuration->RemoveCustomFilter(
          adblock::kAllowlistEverythingFilter);
    }

    AdblockChromeContentBrowserClient::ForceAdblockProxyForTesting();
  }

  bool IsIncognito() { return GetParam() == Mode::Incognito; }

  bool RunTest(const std::string& subtest) {
    const std::string page_url = "main.html?subtest=" + subtest;
    return RunExtensionTest("eyeo_filtering_private",
                            {.extension_url = page_url.c_str()},
                            {.allow_in_incognito = IsIncognito(),
                             .load_as_component = !IsIncognito()});
  }
};

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       CreateRemoveAndGetConfigurations) {
  EXPECT_TRUE(RunTest("createRemoveAndGetConfigurations")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       CreateRemoveAndGetConfigurationsWithPromises) {
  EXPECT_TRUE(RunTest("createRemoveAndGetConfigurationsWithPromises"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       EnableAndDisableConfiguration) {
  EXPECT_TRUE(RunTest("enableAndDisableConfiguration")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       EnableAndDisableConfigurationWithPromises) {
  EXPECT_TRUE(RunTest("enableAndDisableConfigurationWithPromises")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddAllowedDomainToCustomConfiguration) {
  EXPECT_TRUE(RunTest("addAllowedDomainToCustomConfiguration")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddAllowedDomainToCustomConfigurationWithPromises) {
  EXPECT_TRUE(RunTest("addAllowedDomainToCustomConfigurationWithPromises"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddCustomFilterToCustomConfiguration) {
  EXPECT_TRUE(RunTest("addCustomFilterToCustomConfiguration")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddCustomFilterToCustomConfigurationWithPromises) {
  EXPECT_TRUE(RunTest("addCustomFilterToCustomConfigurationWithPromises"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddFilterListInCustomConfiguration) {
  EXPECT_TRUE(RunTest("addFilterListInCustomConfiguration")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       AddFilterListInCustomConfigurationWithPromises) {
  EXPECT_TRUE(RunTest("addFilterListInCustomConfigurationWithPromises"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest, MissingConfiguration) {
  EXPECT_TRUE(RunTest("missingConfiguration")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest,
                       MissingConfigurationWithPromises) {
  EXPECT_TRUE(RunTest("missingConfigurationWithPromises")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest, AllowedDomainsEvent) {
  EXPECT_TRUE(RunTest("allowedDomainsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest, EnabledStateEvent) {
  EXPECT_TRUE(RunTest("enabledStateEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest, FilterListsEvent) {
  EXPECT_TRUE(RunTest("filterListsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(EyeoFilteringPrivateApiTest, CustomFiltersEvent) {
  EXPECT_TRUE(RunTest("customFiltersEvent")) << message_;
}

INSTANTIATE_TEST_SUITE_P(,
                         EyeoFilteringPrivateApiTest,
                         testing::Values(Mode::Normal, Mode::Incognito));

}  // namespace extensions
