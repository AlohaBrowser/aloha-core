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

#ifndef CHROME_BROWSER_EXTENSIONS_API_ADBLOCK_PRIVATE_ADBLOCK_PRIVATE_APITEST_BASE_H_
#define CHROME_BROWSER_EXTENSIONS_API_ADBLOCK_PRIVATE_ADBLOCK_PRIVATE_APITEST_BASE_H_

#include <map>
#include <string>

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class AdblockPrivateApiTestBase : public ExtensionApiTest {
 public:
  enum class Mode { Normal, Incognito };
  enum class EyeoExtensionApi { Old, New };

  AdblockPrivateApiTestBase() {}
  ~AdblockPrivateApiTestBase() override = default;
  AdblockPrivateApiTestBase(const AdblockPrivateApiTestBase&) = delete;
  AdblockPrivateApiTestBase& operator=(const AdblockPrivateApiTestBase&) =
      delete;

  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;

  virtual bool IsIncognito();

  virtual std::string GetApiEndpoint();

  virtual bool RunTest(const std::string& subtest);

  virtual bool RunTestWithParams(
      const std::string& subtest,
      const std::map<std::string, std::string>& params);

 private:
  void AllowTestExtension(base::CommandLine* command_line);

  void EnableIncognitoMode(base::CommandLine* command_line);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_ADBLOCK_PRIVATE_ADBLOCK_PRIVATE_APITEST_BASE_H_
