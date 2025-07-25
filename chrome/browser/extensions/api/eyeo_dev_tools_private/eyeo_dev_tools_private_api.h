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

#ifndef CHROME_BROWSER_EXTENSIONS_API_EYEO_DEV_TOOLS_PRIVATE_EYEO_DEV_TOOLS_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_EYEO_DEV_TOOLS_PRIVATE_EYEO_DEV_TOOLS_PRIVATE_API_H_

#include <tuple>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/devtools/device/android_device_manager.h"
#include "chrome/common/extensions/api/eyeo_dev_tools_private.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class EyeoDevToolsPrivateAPI
    : public BrowserContextKeyedAPI,
      public EventRouter::Observer,
      public AndroidDeviceManager::AndroidWebSocket::Delegate {
 public:
  static BrowserContextKeyedAPIFactory<EyeoDevToolsPrivateAPI>*
  GetFactoryInstance();

  static EyeoDevToolsPrivateAPI* Get(content::BrowserContext* context);

  explicit EyeoDevToolsPrivateAPI(content::BrowserContext* context);
  ~EyeoDevToolsPrivateAPI() override;
  friend class BrowserContextKeyedAPIFactory<EyeoDevToolsPrivateAPI>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "EyeoDevToolsPrivateAPI"; }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsCreatedWithBrowserContext = true;
  void Shutdown() override;

  // EventRouter::Observer:
  void OnListenerAdded(const extensions::EventListenerInfo& details) override;

  // AndroidDeviceManager::AndroidWebSocket::Delegate
  void OnSocketOpened() override;
  void OnFrameRead(const std::string& message) override;
  void OnSocketClosed() override;

  // Socket pointer, socket name, and connection callback fired when socket is
  // connected. When AndroidWebSocket ptr is null other tuple members are not
  // important, this is enough to say there is no remote connection active.
  // When AndroidWebSocket ptr is not null and connection callback is null
  // it is assumed the callback has fired and socket is ready for messages.
  typedef std::tuple<AndroidDeviceManager::AndroidWebSocket*,
                     std::string,
                     std::optional<base::OnceCallback<void()>>>
      Socket;
  Socket& GetSocket() { return socket_; }

  // Map of remote targets to which we can connect to. Keys are socket names,
  // values are Device objects required to establish a socket connection.
  typedef std::map<std::string, scoped_refptr<AndroidDeviceManager::Device>>
      RemoteTargets;
  RemoteTargets& GetRemoteTargets() { return last_targets_; }
  void ClearRemoteTargets() { return last_targets_.clear(); }

  void SendMessage(const std::string& command,
                   const std::string& params,
                   base::OnceCallback<void(const std::string&)> callback);

 private:
  Socket socket_;
  RemoteTargets last_targets_;
  typedef std::map<int, base::OnceCallback<void(const std::string&)>>
      MessagesInProgress;
  MessagesInProgress messages_;
  const raw_ptr<content::BrowserContext> context_;
  class EyeoDevToolsAPIEventRouter;
  std::unique_ptr<EyeoDevToolsAPIEventRouter> event_router_;
};

template <>
void BrowserContextKeyedAPIFactory<
    EyeoDevToolsPrivateAPI>::DeclareFactoryDependencies();

namespace api {

class EyeoDevToolsPrivateConnectFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoDevToolsPrivate.connect", UNKNOWN)
  EyeoDevToolsPrivateConnectFunction();

 private:
  ~EyeoDevToolsPrivateConnectFunction() override;

  ResponseAction Run() override;

  void RespondSocketConnected();

  EyeoDevToolsPrivateConnectFunction(
      const EyeoDevToolsPrivateConnectFunction&) = delete;
  EyeoDevToolsPrivateConnectFunction& operator=(
      const EyeoDevToolsPrivateConnectFunction&) = delete;
};

class EyeoDevToolsPrivateDisconnectFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoDevToolsPrivate.disconnect", UNKNOWN)
  EyeoDevToolsPrivateDisconnectFunction();

 private:
  ~EyeoDevToolsPrivateDisconnectFunction() override;

  ResponseAction Run() override;

  void RespondSocketConnected();

  EyeoDevToolsPrivateDisconnectFunction(
      const EyeoDevToolsPrivateDisconnectFunction&) = delete;
  EyeoDevToolsPrivateDisconnectFunction& operator=(
      const EyeoDevToolsPrivateDisconnectFunction&) = delete;
};

class EyeoDevToolsPrivateRestartDiscoveryFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoDevToolsPrivate.restartDiscovery", UNKNOWN)
  EyeoDevToolsPrivateRestartDiscoveryFunction();

 private:
  ~EyeoDevToolsPrivateRestartDiscoveryFunction() override;

  ResponseAction Run() override;

  void RespondSocketConnected();

  EyeoDevToolsPrivateRestartDiscoveryFunction(
      const EyeoDevToolsPrivateRestartDiscoveryFunction&) = delete;
  EyeoDevToolsPrivateRestartDiscoveryFunction& operator=(
      const EyeoDevToolsPrivateRestartDiscoveryFunction&) = delete;
};

class EyeoDevToolsPrivateSendRemoteCommandFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoDevToolsPrivate.sendRemoteCommand", UNKNOWN)
  EyeoDevToolsPrivateSendRemoteCommandFunction();

 private:
  ~EyeoDevToolsPrivateSendRemoteCommandFunction() override;

  ResponseAction Run() override;

  void RespondWithResult(const std::string& json_string);

  EyeoDevToolsPrivateSendRemoteCommandFunction(
      const EyeoDevToolsPrivateSendRemoteCommandFunction&) = delete;
  EyeoDevToolsPrivateSendRemoteCommandFunction& operator=(
      const EyeoDevToolsPrivateSendRemoteCommandFunction&) = delete;
};

}  // namespace api

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_EYEO_DEV_TOOLS_PRIVATE_EYEO_DEV_TOOLS_PRIVATE_API_H_
