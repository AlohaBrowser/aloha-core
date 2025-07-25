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

#include "chrome/browser/extensions/api/eyeo_dev_tools_private/eyeo_dev_tools_private_api.h"

#include <map>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/devtools/device/devtools_android_bridge.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace extensions {

namespace {

constexpr char kOtherTargetAlreadyConnected[] =
    "There is already another remote target '%s' connected or connecting!";

constexpr char kTargetMissing[] = "Target '%s' is not reachable!";

constexpr char kTargetAlreadyConnected[] = "Connect already called for '%s'!";

content::BrowserContext* GetOriginalBrowserContext(
    content::BrowserContext* browser_context) {
  return Profile::FromBrowserContext(browser_context)->GetOriginalProfile();
}

DevToolsAndroidBridge* GetDevToolsAndroidBridge(
    content::BrowserContext* browser_context) {
  return DevToolsAndroidBridge::Factory::GetForProfile(
      Profile::FromBrowserContext(browser_context)->GetOriginalProfile());
}

AndroidDeviceManager::AndroidWebSocket*& SocketPtr(
    EyeoDevToolsPrivateAPI::Socket& socket) {
  return std::get<0>(socket);
}

std::string& SocketName(EyeoDevToolsPrivateAPI::Socket& socket) {
  return std::get<1>(socket);
}

std::optional<base::OnceCallback<void()>>& SocketReadyCallback(
    EyeoDevToolsPrivateAPI::Socket& socket) {
  return std::get<2>(socket);
}

extensions::api::eyeo_dev_tools_private::RequestInfo CreateRequestInfoObject(
    const std::string& url,
    const std::string& subscription) {
  api::eyeo_dev_tools_private::RequestInfo info;
  info.url = url;
  info.subscription = subscription;
  return info;
}

extensions::api::eyeo_dev_tools_private::PageElementInfo
CreatePageElementInfoObject(const std::string& selector,
                            const std::string& action) {
  api::eyeo_dev_tools_private::PageElementInfo info;
  info.selector = selector;
  info.action = action;
  return info;
}

}  // namespace

class EyeoDevToolsPrivateAPI::EyeoDevToolsAPIEventRouter
    : public DevToolsAndroidBridge::DeviceListListener {
 public:
  explicit EyeoDevToolsAPIEventRouter(content::BrowserContext* context)
      : context_(GetOriginalBrowserContext(context)) {
    auto* bridge = GetDevToolsAndroidBridge(context_);
    CHECK(bridge);
    bridge->AddDeviceListListener(this);
  }

  ~EyeoDevToolsAPIEventRouter() override {
    auto* bridge = GetDevToolsAndroidBridge(context_);
    CHECK(bridge);
    bridge->RemoveDeviceListListener(this);
  }

  void DeviceListChanged(
      const DevToolsAndroidBridge::RemoteDevices& devices) override {
    std::map<std::string, scoped_refptr<AndroidDeviceManager::Device>>
        current_targets;
    auto& last_targets =
        EyeoDevToolsPrivateAPI::Get(context_)->GetRemoteTargets();
    for (const auto& device : devices) {
      if (device->is_connected()) {
        for (const auto& browser : device->browsers()) {
          for (const auto& page : browser->pages()) {
            auto dt_agent_host = page->CreateTarget();
            if (dt_agent_host->GetType() != "page") {
              continue;
            }
            auto socket_target = dt_agent_host->GetId();
            current_targets[socket_target] = page->device();
            if (last_targets.erase(socket_target) > 0) {
              // This isn't a new remote target
              continue;
            }
            VLOG(2) << "[eyeo] Found a new remote target:";
            VLOG(2) << "\t\t[eyeo] page url: " << dt_agent_host->GetURL();
            VLOG(2) << "\t\t[eyeo] page socket: " << socket_target;
            api::eyeo_dev_tools_private::RemoteTargetInfo info =
                CreateRemoteTargetInfoObject(dt_agent_host->GetURL(),
                                             socket_target);
            std::unique_ptr<Event> event = std::make_unique<Event>(
                events::EYEO_EVENT,
                api::eyeo_dev_tools_private::OnRemoteTargetFound::kEventName,
                api::eyeo_dev_tools_private::OnRemoteTargetFound::Create(info));

            extensions::EventRouter::Get(context_)->BroadcastEvent(
                std::move(event));
          }
        }
      }
    }
    for (const auto& target : last_targets) {
      VLOG(2) << "[eyeo] Lost a remote target:";
      VLOG(2) << "\t\t[eyeo] page socket: " << target.first;
      api::eyeo_dev_tools_private::RemoteTargetInfo info =
          CreateRemoteTargetInfoObject(GURL{}, target.first);
      std::unique_ptr<Event> event = std::make_unique<Event>(
          events::EYEO_EVENT,
          api::eyeo_dev_tools_private::OnRemoteTargetLost::kEventName,
          api::eyeo_dev_tools_private::OnRemoteTargetLost::Create(info));

      extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
    }
    last_targets.swap(current_targets);
  }

 private:
  api::eyeo_dev_tools_private::RemoteTargetInfo CreateRemoteTargetInfoObject(
      const GURL& page_url,
      const std::string& socket_name) {
    api::eyeo_dev_tools_private::RemoteTargetInfo info;
    info.page_url = page_url.spec();
    info.socket_name = socket_name;
    return info;
  }

  const raw_ptr<content::BrowserContext> context_;
};

template <>
void BrowserContextKeyedAPIFactory<
    EyeoDevToolsPrivateAPI>::DeclareFactoryDependencies() {
  DependsOn(DevToolsAndroidBridge::Factory::GetInstance());
}

// static
BrowserContextKeyedAPIFactory<EyeoDevToolsPrivateAPI>*
EyeoDevToolsPrivateAPI::GetFactoryInstance() {
  static base::NoDestructor<
      BrowserContextKeyedAPIFactory<EyeoDevToolsPrivateAPI>>
      instance;
  return instance.get();
}

// static
EyeoDevToolsPrivateAPI* EyeoDevToolsPrivateAPI::Get(
    content::BrowserContext* context) {
  return GetFactoryInstance()->Get(context);
}

EyeoDevToolsPrivateAPI::EyeoDevToolsPrivateAPI(content::BrowserContext* context)
    : context_(context) {
  // EventRouter can be null in tests
  auto* ev = EventRouter::Get(context_);
  if (ev) {
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnPageAllowed::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnRemoteTargetFound::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnRemoteTargetLost::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnRequestAllowed::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnRequestBlocked::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_dev_tools_private::OnPageElementMatched::kEventName);
  }
  socket_ = std::make_tuple(nullptr, "", std::nullopt);
}

EyeoDevToolsPrivateAPI::~EyeoDevToolsPrivateAPI() {
  if (SocketPtr(socket_)) {
    delete SocketPtr(socket_);
    SocketPtr(socket_) = nullptr;
  }
}

void EyeoDevToolsPrivateAPI::Shutdown() {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->UnregisterObserver(this);
  }
  event_router_.reset();
}

void EyeoDevToolsPrivateAPI::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  event_router_ =
      std::make_unique<EyeoDevToolsPrivateAPI::EyeoDevToolsAPIEventRouter>(
          context_);
  EventRouter::Get(context_)->UnregisterObserver(this);
}

void EyeoDevToolsPrivateAPI::SendMessage(
    const std::string& command,
    const std::string& params,
    base::OnceCallback<void(const std::string&)> callback) {
  static int s_message_id = 0;
  static constexpr char s_message_tmpl[] =
      "{\"id\":%d,\"method\":\"Eyeo.%s\",\"params\":%s}";
  DCHECK(SocketPtr(socket_));
  auto next_massage_id = s_message_id++;
  messages_[next_massage_id] = std::move(callback);
  auto message = base::StringPrintf(s_message_tmpl, next_massage_id, command,
                                    params.empty() ? "{}" : params);
  SocketPtr(socket_)->SendFrame(message);
}

void EyeoDevToolsPrivateAPI::OnSocketOpened() {
  VLOG(2) << "[eyeo] EyeoDevToolsPrivateAPI::OnSocketOpened: "
          << SocketName(socket_);
  DCHECK(SocketReadyCallback(socket_).has_value());
  if (SocketReadyCallback(socket_).has_value()) {
    std::move(SocketReadyCallback(socket_).value()).Run();
    SocketReadyCallback(socket_) = std::nullopt;
  }
}

void EyeoDevToolsPrivateAPI::OnFrameRead(const std::string& message) {
  VLOG(2) << "[eyeo] EyeoDevToolsPrivateAPI::OnFrameRead: " << message;
  std::optional<base::Value::Dict> json = base::JSONReader::ReadDict(message);
  if (json) {
    if (json->FindInt("id")) {
      auto key = json->FindInt("id").value();
      auto cb_entry = messages_.find(key);
      DCHECK(cb_entry != messages_.end());
      if (cb_entry != messages_.end()) {
        std::move(messages_[key]).Run(message);
        messages_.erase(cb_entry);
      }
    } else if (json->FindString("method") && json->FindDict("params")) {
      auto action = *json->FindString("method");
      if (action == "Eyeo.pageAllowed" || action == "Eyeo.requestAllowed" ||
          action == "Eyeo.requestBlocked") {
        auto* params = json->FindDict("params");
        auto url = (params->FindString("url") ? *params->FindString("url")
                                              : std::string("<?>"));
        auto subscription = (params->FindString("subscription")
                                 ? *params->FindString("subscription")
                                 : std::string("<?>"));
        api::eyeo_dev_tools_private::RequestInfo info =
            CreateRequestInfoObject(url, subscription);
        std::unique_ptr<Event> event;
        if (action == "Eyeo.requestAllowed") {
          event = std::make_unique<Event>(
              events::EYEO_EVENT,
              api::eyeo_dev_tools_private::OnRequestAllowed::kEventName,
              api::eyeo_dev_tools_private::OnRequestAllowed::Create(info));
        } else if (action == "Eyeo.requestBlocked") {
          event = std::make_unique<Event>(
              events::EYEO_EVENT,
              api::eyeo_dev_tools_private::OnRequestBlocked::kEventName,
              api::eyeo_dev_tools_private::OnRequestBlocked::Create(info));
        } else {
          event = std::make_unique<Event>(
              events::EYEO_EVENT,
              api::eyeo_dev_tools_private::OnPageAllowed::kEventName,
              api::eyeo_dev_tools_private::OnPageAllowed::Create(info));
        }
        extensions::EventRouter::Get(context_)->BroadcastEvent(
            std::move(event));
      } else if (action == "Eyeo.pageElementMatched") {
        auto* params = json->FindDict("params");
        auto selector =
            (params->FindString("selector") ? *params->FindString("selector")
                                            : std::string("<?>"));
        auto eh_action =
            (params->FindString("action") ? *params->FindString("action")
                                          : std::string("<?>"));
        api::eyeo_dev_tools_private::PageElementInfo info =
            CreatePageElementInfoObject(selector, eh_action);
        std::unique_ptr<Event> event = std::make_unique<Event>(
            events::EYEO_EVENT,
            api::eyeo_dev_tools_private::OnPageElementMatched::kEventName,
            api::eyeo_dev_tools_private::OnPageElementMatched::Create(info));
        extensions::EventRouter::Get(context_)->BroadcastEvent(
            std::move(event));
      }
    }
  }
}

void EyeoDevToolsPrivateAPI::OnSocketClosed() {
  VLOG(2) << "[eyeo] EyeoDevToolsPrivateAPI::OnSocketClosed: "
          << SocketName(socket_);
  delete SocketPtr(socket_);
  SocketPtr(socket_) = nullptr;
}

namespace api {

ExtensionFunction::ResponseAction EyeoDevToolsPrivateConnectFunction::Run() {
  std::optional<eyeo_dev_tools_private::Connect::Params> params(
      eyeo_dev_tools_private::Connect::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto socket_name = params->socket_name;
  auto* extension = EyeoDevToolsPrivateAPI::Get(browser_context());
  auto& socket = extension->GetSocket();
  // Check if we are already connected or connecting
  if (SocketPtr(socket)) {
    if (SocketName(socket) == socket_name) {
      return RespondNow(Error(
          base::StringPrintf(kTargetAlreadyConnected, socket_name.c_str())));
    } else {
      return RespondNow(Error(base::StringPrintf(kOtherTargetAlreadyConnected,
                                                 socket_name.c_str())));
    }
  }
  auto& remote_targets = extension->GetRemoteTargets();
  if (!remote_targets.count(socket_name)) {
    return RespondNow(
        Error(base::StringPrintf(kTargetMissing, socket_name.c_str())));
  }

  SocketReadyCallback(socket) = base::BindOnce(
      &EyeoDevToolsPrivateConnectFunction::RespondSocketConnected, this);

  SocketName(socket) = socket_name;
  std::vector<std::string> socket_tokens = base::SplitString(
      socket_name, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  SocketPtr(socket) = remote_targets[socket_name]->CreateWebSocket(
      socket_tokens[1], "/devtools/page/" + socket_tokens[2], extension);

  return RespondLater();
}

void EyeoDevToolsPrivateConnectFunction::RespondSocketConnected() {
  Respond(NoArguments());
}

EyeoDevToolsPrivateConnectFunction::EyeoDevToolsPrivateConnectFunction() =
    default;

EyeoDevToolsPrivateConnectFunction::~EyeoDevToolsPrivateConnectFunction() =
    default;

ExtensionFunction::ResponseAction EyeoDevToolsPrivateDisconnectFunction::Run() {
  auto* extension = EyeoDevToolsPrivateAPI::Get(browser_context());
  auto& socket = extension->GetSocket();
  std::string response;
  if (SocketPtr(socket)) {
    delete SocketPtr(socket);
    response = SocketName(socket);
    socket = std::make_tuple(nullptr, "", std::nullopt);
  }
  return RespondNow(ArgumentList(
      eyeo_dev_tools_private::Disconnect::Results::Create(response)));
}

EyeoDevToolsPrivateDisconnectFunction::EyeoDevToolsPrivateDisconnectFunction() =
    default;

EyeoDevToolsPrivateDisconnectFunction::
    ~EyeoDevToolsPrivateDisconnectFunction() = default;

ExtensionFunction::ResponseAction
EyeoDevToolsPrivateRestartDiscoveryFunction::Run() {
  auto* extension = EyeoDevToolsPrivateAPI::Get(browser_context());
  extension->ClearRemoteTargets();
  return RespondNow(NoArguments());
}

EyeoDevToolsPrivateRestartDiscoveryFunction::
    EyeoDevToolsPrivateRestartDiscoveryFunction() = default;

EyeoDevToolsPrivateRestartDiscoveryFunction::
    ~EyeoDevToolsPrivateRestartDiscoveryFunction() = default;

ExtensionFunction::ResponseAction
EyeoDevToolsPrivateSendRemoteCommandFunction::Run() {
  std::optional<eyeo_dev_tools_private::SendRemoteCommand::Params> params(
      eyeo_dev_tools_private::SendRemoteCommand::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto method = params->method;
  auto params_json = params->params_json_string.value_or("");
  auto* extension = EyeoDevToolsPrivateAPI::Get(browser_context());
  auto& socket = extension->GetSocket();

  if (!SocketPtr(socket)) {
    return RespondNow(Error("Remote target not connected!"));
  }

  extension->SendMessage(
      method, params_json,
      base::BindOnce(
          &EyeoDevToolsPrivateSendRemoteCommandFunction::RespondWithResult,
          this));

  return RespondLater();
}

void EyeoDevToolsPrivateSendRemoteCommandFunction::RespondWithResult(
    const std::string& json_string) {
  return Respond(ArgumentList(
      eyeo_dev_tools_private::SendRemoteCommand::Results::Create(json_string)));
}

EyeoDevToolsPrivateSendRemoteCommandFunction::
    EyeoDevToolsPrivateSendRemoteCommandFunction() = default;

EyeoDevToolsPrivateSendRemoteCommandFunction::
    ~EyeoDevToolsPrivateSendRemoteCommandFunction() = default;

}  // namespace api

}  // namespace extensions
