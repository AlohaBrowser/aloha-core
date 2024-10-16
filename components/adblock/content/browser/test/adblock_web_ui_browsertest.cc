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

#include "base/strings/strcat.h"
#include "components/adblock/core/common/web_ui_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/app/shell_main_delegate.h"
#include "content/shell/browser/shell.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockWebUIBrowserTest : public content::ContentBrowserTest {
 public:
  // Without this override there is no AdblockShellContentBrowserClient
  // (created by ShellMainDelegate) but default ShellContentBrowserClient.
  content::ContentMainDelegate* GetOptionalContentMainDelegateOverride()
      override {
    return new content::ShellMainDelegate(true);
  }
};

IN_PROC_BROWSER_TEST_F(AdblockWebUIBrowserTest,
                       AdblockInternalsPageLoadsSuccessfully) {
  ASSERT_TRUE(content::NavigateToURL(
      shell(),
      GURL(base::StrCat({"chrome://", kChromeUIAdblockInternalsHost}))));
  EXPECT_TRUE(shell()->web_contents()->GetWebUI());
  EXPECT_EQ(u"Ad-Filtering Internals", shell()->web_contents()->GetTitle());
}

IN_PROC_BROWSER_TEST_F(
    AdblockWebUIBrowserTest,
    VisitingAdblockInternalsWithInvalidSchemeResultsInErrorPage) {
  EXPECT_FALSE(content::NavigateToURL(
      shell(), GURL(base::StrCat({"http://", kChromeUIAdblockInternalsHost}))));
  EXPECT_FALSE(shell()->web_contents()->GetWebUI());
  EXPECT_EQ(u"Error", shell()->web_contents()->GetTitle());
}

}  // namespace adblock
