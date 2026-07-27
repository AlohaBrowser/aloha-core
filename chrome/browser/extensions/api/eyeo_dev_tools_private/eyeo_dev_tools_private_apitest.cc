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

#include "base/strings/stringprintf.h"
#include "chrome/browser/devtools/device/adb/adb_device_provider.h"
#include "chrome/browser/devtools/device/adb/mock_adb_server.h"
#include "chrome/browser/devtools/device/devtools_android_bridge.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {
void AllowTestExtension(base::CommandLine* command_line) {
  command_line->AppendSwitchASCII(extensions::switches::kAllowlistedExtensionID,
                                  "ocidpehmkefohkcpefepbccbliaipofj");
}
}  // namespace

class EyeoDevToolsPrivateApiTest
    : public ExtensionApiTest,
      public DevToolsAndroidBridge::DeviceListListener {
 public:
  EyeoDevToolsPrivateApiTest() {}
  ~EyeoDevToolsPrivateApiTest() override = default;
  EyeoDevToolsPrivateApiTest(const EyeoDevToolsPrivateApiTest&) = delete;
  EyeoDevToolsPrivateApiTest& operator=(const EyeoDevToolsPrivateApiTest&) =
      delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
    AllowTestExtension(command_line);
  }

  void StartTest(base::RunLoop* loop) {
    auto* android_bridge =
        DevToolsAndroidBridge::Factory::GetForProfile(browser()->profile());
    AndroidDeviceManager::DeviceProviders device_providers;
    device_providers.push_back(new AdbDeviceProvider());
    android_bridge->set_device_providers_for_test(device_providers);
    android_bridge->AddDeviceListListener(this);
    loop_ = loop;
    loop_->Run();
  }

  void DeviceListChanged(
      const DevToolsAndroidBridge::RemoteDevices& devices) override {
    // Devices count is fixed in MockAdbServer
    if (devices.size() == 2) {
      auto* android_bridge =
          DevToolsAndroidBridge::Factory::GetForProfile(browser()->profile());
      android_bridge->RemoveDeviceListListener(this);
      loop_->QuitWhenIdle();
    }
  }

  int ExecuteScriptAndGetInt(const std::string& extension_id,
                             const std::string& script) {
    return ExtensionApiTest::ExecuteScriptInBackgroundPage(extension_id, script)
        .GetInt();
  }

  const ExtensionId& GetExtensionId() const { return extension_->id(); }

 protected:
  raw_ptr<base::RunLoop> loop_;
  raw_ptr<const Extension, DanglingUntriaged> extension_;
};

IN_PROC_BROWSER_TEST_F(EyeoDevToolsPrivateApiTest, DiscoverRemoteTargets) {
  extension_ =
      LoadExtension(test_data_dir_.AppendASCII("eyeo_dev_tools_private"));
  ASSERT_TRUE(extension_);

  constexpr char discovery_script[] = R"(
      let testData = {};
      testData.foundCount = 0;
      testData.lostCount = 0;
      chrome.eyeoDevToolsPrivate.onRemoteTargetFound.addListener((info) => {
        console.log("Found: " + JSON.stringify(info));
        testData.foundCount = testData.foundCount + 1;
      });
      chrome.eyeoDevToolsPrivate.onRemoteTargetLost.addListener((info) => {
        console.log("Lost: " + JSON.stringify(info));
        testData.lostCount = testData.lostCount + 1;
      });
      chrome.test.sendScriptResult(0);
  )";

  constexpr char verify_counters_tmpl[] = R"(
        var intervalId = setInterval(function() {
          let result = testData.%s;
          if (result == 4) {
            if (intervalId) {
              clearInterval(intervalId);
              intervalId = null;
            }
            chrome.test.sendScriptResult(result);
          }
        }, 100);
  )";

  EXPECT_EQ(0, ExecuteScriptAndGetInt(GetExtensionId(), discovery_script));
  base::RunLoop loop;
  StartMockAdbServer(FlushWithoutSize);
  StartTest(&loop);
  StopMockAdbServer();

  // Hardcoded data in MockAdbServer contains 4 remote "page" targets
  EXPECT_EQ(4, ExecuteScriptAndGetInt(
                   GetExtensionId(),
                   base::StringPrintf(verify_counters_tmpl, "foundCount")));
  EXPECT_EQ(4, ExecuteScriptAndGetInt(
                   GetExtensionId(),
                   base::StringPrintf(verify_counters_tmpl, "lostCount")));
}

// This test fails with MockAdbServer but passes with a remote WebViewShell app
// running on emulator.
IN_PROC_BROWSER_TEST_F(EyeoDevToolsPrivateApiTest,
                       DISABLED_ConnectToRemoteTarget) {
  extension_ =
      LoadExtension(test_data_dir_.AppendASCII("eyeo_dev_tools_private"));
  ASSERT_TRUE(extension_);

  constexpr char connect_script[] = R"(
      let selected = false;
      chrome.eyeoDevToolsPrivate.onRemoteTargetFound.addListener((info) => {
          if (selected) {
            return;
          }
          selected = true;
          console.log("Found: " + JSON.stringify(info));
          chrome.eyeoDevToolsPrivate.connect(info.socket_name, () => {
              if (chrome.runtime.lastError) {
                console.log(chrome.runtime.lastError.message);
                chrome.test.sendScriptResult(1);
                return;
              }
              console.log("Connected to: " + info.socket_name);
              // Confirm that 2nd connection attempt is rejected
              chrome.eyeoDevToolsPrivate.connect(info.socket_name, () => {
                  if (!chrome.runtime.lastError) {
                    chrome.test.sendScriptResult(2);
                    return;
                  }
                  console.log(chrome.runtime.lastError.message);
                  chrome.eyeoDevToolsPrivate.disconnect((socket_name) => {
                      if (!socket_name || (socket_name != info.socket_name)) {
                        console.log(chrome.runtime.lastError.message);
                        chrome.test.sendScriptResult(3);
                        return;
                      }
                      console.log("Disconnected from: " + info.socket_name);
                      // Confirm that now 2nd connection attempt is granted
                      chrome.eyeoDevToolsPrivate.connect(info.socket_name, () => {
                          if (chrome.runtime.lastError) {
                            console.log(chrome.runtime.lastError.message);
                            chrome.test.sendScriptResult(4);
                            return;
                          }
                          console.log("Connected again to: " + info.socket_name);
                          chrome.test.sendScriptResult(0);
                      });
                  });
              });
          });
      });
  )";

  EXPECT_EQ(0, ExecuteScriptAndGetInt(GetExtensionId(), connect_script));
}

// This test fails with MockAdbServer but passes with a remote WebViewShell app
// running on emulator.
IN_PROC_BROWSER_TEST_F(EyeoDevToolsPrivateApiTest,
                       DISABLED_SendCommandToRemoteTarget) {
  extension_ =
      LoadExtension(test_data_dir_.AppendASCII("eyeo_dev_tools_private"));
  ASSERT_TRUE(extension_);

  constexpr char command_script[] = R"(
      let selected = false;
      chrome.eyeoDevToolsPrivate.onRemoteTargetFound.addListener((info) => {
          if (selected) {
            return;
          }
          selected = true;
          console.log("Found: " + JSON.stringify(info));
          chrome.eyeoDevToolsPrivate.connect(info.socket_name, () => {
              console.log("Connected: " + info.socket_name);
              chrome.eyeoDevToolsPrivate.sendRemoteCommand("addCustomFilter", "{\"filter\": \"dummy\"}", (result) => {
                  if (chrome.runtime.lastError) {
                    console.log(chrome.runtime.lastError.message);
                    chrome.test.sendScriptResult(1);
                    return;
                  }
                  console.log("Command output: " + result);
                  chrome.eyeoDevToolsPrivate.sendRemoteCommand("getCustomFilters", (result) => {
                      if (chrome.runtime.lastError) {
                        console.log(chrome.runtime.lastError.message);
                        chrome.test.sendScriptResult(2);
                        return;
                      }
                      console.log("Command output: " + result);
                      if (result !== "{\"id\":1,\"result\":{\"filters\":[\"dummy\"]}}") {
                        chrome.test.sendScriptResult(3);
                        return;
                      }
                      chrome.eyeoDevToolsPrivate.sendRemoteCommand("removeCustomFilter", "{\"filter\": \"dummy\"}", (result) => {
                          if (chrome.runtime.lastError) {
                            console.log(chrome.runtime.lastError.message);
                            chrome.test.sendScriptResult(4);
                            return;
                          }
                          console.log("Command output: " + result);
                          chrome.eyeoDevToolsPrivate.sendRemoteCommand("getCustomFilters", (result) => {
                              if (chrome.runtime.lastError) {
                                console.log(chrome.runtime.lastError.message);
                                chrome.test.sendScriptResult(5);
                                return;
                              }
                              console.log("Command output: " + result);
                              if (result !== "{\"id\":3,\"result\":{\"filters\":[]}}") {
                                chrome.test.sendScriptResult(6);
                                return;
                              }
                              chrome.test.sendScriptResult(0);
                          });
                      });
                  });
              });
          });
      });
  )";

  EXPECT_EQ(0, ExecuteScriptAndGetInt(GetExtensionId(), command_script));
}

}  // namespace extensions
