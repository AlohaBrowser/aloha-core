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

#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/url_canon.h"
#include "url/url_util.h"

namespace adblock {

namespace {

constexpr char kResponseOk[] = "OK";
constexpr char kResponseInvalidCommand[] = "INVALID_COMMAND";
constexpr char kResponseInvalidConfiguration[] =
    "Configuration '%s' does not exist!";
constexpr char kResponseHelpListingConfigurations[] =
    "Type '%s' to "
    "list existing configurations.";
constexpr char kTopicFilters[] = "filters";
constexpr char kTopicDomains[] = "domains";
constexpr char kTopicSubscriptions[] = "subscriptions";
constexpr char kTopicConfiguration[] = "configuration";
constexpr char kTopicAcceptableAds[] = "aa";
constexpr char kTopicHelp[] = "help";

constexpr char kActionAdd[] = "add";
constexpr char kActionClear[] = "clear";
constexpr char kActionList[] = "list";
constexpr char kActionRemove[] = "remove";
constexpr char kActionEnable[] = "enable";
constexpr char kActionDisable[] = "disable";
constexpr char kActionState[] = "state";

constexpr char kListConfigurations[] = "test.data/configurations";

constexpr char kFilteringConfigurationCommandHelp[] = R"(
  Command syntax `%s.test.data/[topic]/[action]/[payload]` where:
  - `topic` is either `domains` (allowed domains), `filters`, `subscriptions`,
    `configuration`, `aa` (Acceptable Ads)
  - `action` is either:
      - `add`, `clear` (remove all), `list`, `remove` valid for `domains`,
        `filters` and `subscriptions`
      - `enable`, `disable` or `state` valid for `aa` and `configuration`
  - `payload` is url encoded string required for action `add` and `remove`.
  When adding or removing filter/domain/subscription one can encode several
  entries splitting them by a new line character.
)";

std::string GetFilteringConfigurationCommandsHelpMessage(
    const char* configuration_name) {
  return base::StringPrintf(kFilteringConfigurationCommandHelp,
                            configuration_name);
}

std::string GetInvalidFilteringConfigurationWithHelpMessage(
    const char* configuration_name) {
  return base::StringPrintf(kResponseInvalidConfiguration, configuration_name) +
         "\n\n" +
         base::StringPrintf(kResponseHelpListingConfigurations,
                            kListConfigurations);
}

std::string GetBasicHelpMessage() {
  return GetFilteringConfigurationCommandsHelpMessage("[configuration]") +
         "\n\n" +
         base::StringPrintf(kResponseHelpListingConfigurations,
                            kListConfigurations);
}

std::string GetInvalidCommandWithHelpMessage(const char* configuration_name) {
  return std::string(kResponseInvalidCommand) + "\n\n" +
         GetFilteringConfigurationCommandsHelpMessage(configuration_name);
}

constexpr char payload_delimiter[] = "\n";

std::string DecodePayload(const std::string& encoded) {
  // Example how to encode:
  // url::RawCanonOutputT<char> buffer;
  // url::EncodeURIComponent(base.c_str(), base.size(), &buffer);
  // std::string encoded(buffer.data(), buffer.length());
  VLOG(2) << "[eyeo] Encoded payload: " << encoded;
  url::RawCanonOutputT<char16_t> output;
  url::DecodeURLEscapeSequences(encoded, url::DecodeURLMode::kUTF8OrIsomorphic,
                                &output);
  std::string decoded =
      base::UTF16ToUTF8(std::u16string(output.data(), output.length()));
  VLOG(2) << "[eyeo] Decoded payload: " << decoded;
  return decoded;
}

std::vector<std::string> GetCommandElements(const GURL& url) {
  std::vector<std::string> command_elements;
  auto path = url.path();
  if (path.length() > 1) {
    path.erase(0, 1);
    command_elements = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                         base::SPLIT_WANT_NONEMPTY);
  }
  return command_elements;
}

void Clear(FilteringConfiguration* configuration,
           std::vector<std::string> (FilteringConfiguration::*getter)() const,
           void (FilteringConfiguration::*action)(const std::string&)) {
  for (const auto& item : (configuration->*getter)()) {
    (configuration->*action)(item);
  }
}

std::vector<std::string> List(
    FilteringConfiguration* configuration,
    std::vector<std::string> (FilteringConfiguration::*getter)() const) {
  return (configuration->*getter)();
}

void AddOrRemove(FilteringConfiguration* configuration,
                 void (FilteringConfiguration::*action)(const std::string&),
                 std::string& items) {
  auto items_list = base::SplitString(items, payload_delimiter,
                                      base::WhitespaceHandling::TRIM_WHITESPACE,
                                      base::SplitResult::SPLIT_WANT_NONEMPTY);
  for (const auto& item : items_list) {
    (configuration->*action)(item);
  }
}

void ClearSubscriptions(FilteringConfiguration* configuration) {
  for (const auto& subscription : configuration->GetFilterLists()) {
    configuration->RemoveFilterList(subscription);
  }
}

std::vector<std::string> ListSubscriptions(
    FilteringConfiguration* configuration) {
  std::vector<std::string> subscriptions;
  base::ranges::transform(
      configuration->GetFilterLists(), std::back_inserter(subscriptions),
      [](const auto& subscription) { return subscription.spec(); });
  return subscriptions;
}

void AddOrRemoveSubscription(
    FilteringConfiguration* configuration,
    void (FilteringConfiguration::*action)(const GURL&),
    std::string& items) {
  auto items_list = base::SplitString(items, payload_delimiter,
                                      base::WhitespaceHandling::TRIM_WHITESPACE,
                                      base::SplitResult::SPLIT_WANT_NONEMPTY);
  for (const auto& item : items_list) {
    (configuration->*action)(GURL{item});
  }
}

bool IsAdblockEnabled(FilteringConfiguration* configuration) {
  return configuration->IsEnabled();
}

void SetAdblockEnabled(FilteringConfiguration* configuration, bool enabled) {
  configuration->SetEnabled(enabled);
}

bool IsAAEnabled(FilteringConfiguration* configuration) {
  return base::ranges::any_of(
      configuration->GetFilterLists(),
      [&](const auto& url) { return url == AcceptableAdsUrl(); });
}

void SetAAEnabled(FilteringConfiguration* configuration, bool enabled) {
  enabled ? configuration->AddFilterList(AcceptableAdsUrl())
          : configuration->RemoveFilterList(AcceptableAdsUrl());
}

}  // namespace

// static
const char AdblockURLLoaderFactoryForTest::kEyeoDebugDataHostName[] =
    "test.data";

AdblockURLLoaderFactoryForTest::AdblockURLLoaderFactoryForTest(
    AdblockURLLoaderFactoryConfig config,
    RequestInitiator request_initiator,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    std::string user_agent_string,
    DisconnectCallback on_disconnect,
    SubscriptionService* subscription_service)
    : AdblockURLLoaderFactory(std::move(config),
                              std::move(request_initiator),
                              std::move(receiver),
                              std::move(target_factory),
                              user_agent_string,
                              std::move(on_disconnect)),
      subscription_service_(subscription_service) {}

AdblockURLLoaderFactoryForTest::~AdblockURLLoaderFactoryForTest() = default;

void AdblockURLLoaderFactoryForTest::CreateLoaderAndStart(
    ::mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK(subscription_service_);
  if (!url::DomainIs(request.url.host_piece(), kEyeoDebugDataHostName)) {
    DLOG(WARNING)
        << "[eyeo] AdblockURLLoaderFactoryForTest got unexpected url: "
        << request.url;
    AdblockURLLoaderFactory::CreateLoaderAndStart(
        std::move(loader), request_id, options, request, std::move(client),
        traffic_annotation);
    return;
  }
  VLOG(2) << "[eyeo] AdblockURLLoaderFactoryForTest handles: " << request.url;
  std::string response_body;
  if (!base::StartsWith(request.url.host_piece(), kEyeoDebugDataHostName)) {
    configuration_name_ = request.url.host_piece().substr(
        0, request.url.host_piece().rfind(kEyeoDebugDataHostName) - 1);
    response_body = HandleCommand(request.url);
  } else if (base::EndsWith(request.url.spec(), kListConfigurations)) {
    response_body = ListConfigurations();
  } else {
    response_body = GetBasicHelpMessage();
  }
  SendResponse(std::move(response_body), std::move(client));
}

void AdblockURLLoaderFactoryForTest::SendResponse(
    std::string response_body,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> client) const {
  auto response_head = network::mojom::URLResponseHead::New();
  response_head->mime_type = "text/plain";
  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client));
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
    DLOG(ERROR)
        << "[eyeo] AdblockURLLoaderFactoryForTest fails to call CreateDataPipe";
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
    return;
  }
  size_t write_size = response_body.size();
  producer->WriteData(base::as_byte_span(response_body),
                      MOJO_WRITE_DATA_FLAG_NONE, write_size);
  client_remote->OnReceiveResponse(std::move(response_head),
                                   std::move(consumer), absl::nullopt);
  network::URLLoaderCompletionStatus status;
  status.error_code = net::OK;
  status.decoded_body_length = write_size;
  client_remote->OnComplete(status);
}

std::string AdblockURLLoaderFactoryForTest::ListConfigurations() const {
  std::string response = kResponseOk;
  response += "\n\n";
  for (const auto* config :
       subscription_service_->GetInstalledFilteringConfigurations()) {
    response += config->GetName() + "\n";
  }
  return response;
}

std::string AdblockURLLoaderFactoryForTest::HandleCommand(
    const GURL& url) const {
  auto* configuration = GetConfiguration();
  if (!configuration) {
    return GetInvalidFilteringConfigurationWithHelpMessage(
        configuration_name_.c_str());
  }
  auto command_elements = GetCommandElements(url);
  if (command_elements.size() == 1 && command_elements[0] == kTopicHelp) {
    return GetFilteringConfigurationCommandsHelpMessage(
        configuration_name_.c_str());
  }
  // There needs to be at least topic and action, plus optional payload
  if (command_elements.size() > 1) {
    const auto& topic = command_elements[0];
    const auto& action = command_elements[1];
    if (topic == kTopicSubscriptions) {
      if (command_elements.size() == 3) {
        // This can be either add or remove with custom payload
        void (FilteringConfiguration::*action_ptr)(const GURL&) = nullptr;
        std::string payload = DecodePayload(command_elements[2]);
        if (action == kActionAdd) {
          action_ptr = &FilteringConfiguration::AddFilterList;
        } else if (action == kActionRemove) {
          action_ptr = &FilteringConfiguration::RemoveFilterList;
        }
        if (action_ptr) {
          VLOG(2) << "[eyeo] Handling subscription payload: " << payload;
          AddOrRemoveSubscription(configuration, action_ptr, payload);
          return kResponseOk;
        }
      } else if (action == kActionClear) {
        ClearSubscriptions(configuration);
        return kResponseOk;
      } else if (action == kActionList) {
        std::string response = kResponseOk;
        auto subscriptions = ListSubscriptions(configuration);
        if (!subscriptions.empty()) {
          response += "\n\n";
          for (const auto& subscription : subscriptions) {
            response += subscription + "\n";
          }
        }
        return response;
      }
    } else if (topic == kTopicFilters || topic == kTopicDomains) {
      if (command_elements.size() == 3) {
        // This can be either add or remove with custom payload
        void (FilteringConfiguration::*action_ptr)(const std::string&) =
            nullptr;
        std::string payload = DecodePayload(command_elements[2]);
        if (topic == kTopicFilters) {
          if (action == kActionAdd) {
            action_ptr = &FilteringConfiguration::AddCustomFilter;
          } else if (action == kActionRemove) {
            action_ptr = &FilteringConfiguration::RemoveCustomFilter;
          }
        } else {
          if (action == kActionAdd) {
            action_ptr = &FilteringConfiguration::AddAllowedDomain;
          } else if (action == kActionRemove) {
            action_ptr = &FilteringConfiguration::RemoveAllowedDomain;
          }
        }
        if (action_ptr) {
          VLOG(2) << "[eyeo] Handling payload: " << payload;
          AddOrRemove(configuration, action_ptr, payload);
          return kResponseOk;
        }
      } else {
        std::vector<std::string> (FilteringConfiguration::*getter)() const =
            nullptr;
        void (FilteringConfiguration::*deleter)(const std::string&) = nullptr;
        if (action == kActionClear) {
          if (topic == kTopicFilters) {
            deleter = &FilteringConfiguration::RemoveCustomFilter;
            getter = &FilteringConfiguration::GetCustomFilters;
          } else {
            deleter = &FilteringConfiguration::RemoveAllowedDomain;
            getter = &FilteringConfiguration::GetAllowedDomains;
          }
        } else if (action == kActionList) {
          if (topic == kTopicFilters) {
            getter = &FilteringConfiguration::GetCustomFilters;
          } else {
            getter = &FilteringConfiguration::GetAllowedDomains;
          }
        }
        if (deleter && getter) {
          Clear(configuration, getter, deleter);
          return kResponseOk;
        } else if (getter) {
          std::string response = kResponseOk;
          auto items = List(configuration, getter);
          if (!items.empty()) {
            response += "\n\n";
            for (const auto& item : items) {
              response += item + "\n";
            }
          }
          return response;
        }
      }
    } else if (topic == kTopicConfiguration || topic == kTopicAcceptableAds) {
      if (action == kActionState) {
        std::string response = kResponseOk;
        response += "\n\n";
        bool enabled = topic == kTopicConfiguration
                           ? IsAdblockEnabled(configuration)
                           : IsAAEnabled(configuration);
        response += enabled ? "enabled" : "disabled";
        return response;
      }
      absl::optional<bool> value = absl::nullopt;
      if (action == kActionEnable) {
        value = true;
      } else if (action == kActionDisable) {
        value = false;
      }
      if (value.has_value()) {
        if (topic == kTopicConfiguration) {
          SetAdblockEnabled(configuration, value.value());
        } else {
          SetAAEnabled(configuration, value.value());
        }
        return kResponseOk;
      }
    }
  }
  return GetInvalidCommandWithHelpMessage(configuration_name_.c_str());
}

FilteringConfiguration* AdblockURLLoaderFactoryForTest::GetConfiguration()
    const {
  return subscription_service_->GetFilteringConfiguration(configuration_name_);
}

}  // namespace adblock
