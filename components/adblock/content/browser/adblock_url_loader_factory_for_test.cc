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
#include "base/strings/utf_string_conversions.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
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
constexpr char kTopicFilters[] = "filters";
constexpr char kTopicDomains[] = "domains";
constexpr char kTopicSubscriptions[] = "subscriptions";
constexpr char kTopicAdblock[] = "adblock";
constexpr char kTopicAcceptableAds[] = "aa";
constexpr char kTopicHelp[] = "help";

constexpr char kActionAdd[] = "add";
constexpr char kActionClear[] = "clear";
constexpr char kActionList[] = "list";
constexpr char kActionRemove[] = "remove";
constexpr char kActionEnable[] = "enable";
constexpr char kActionDisable[] = "disable";
constexpr char kActionState[] = "state";

constexpr char kHelp[] = R"(
  Command syntax `adblock.test.data/[topic]/[action]/[payload]` where:
  - `topic` is either `domains` (allowed domains), `filters`, `subscriptions`,
    `adblock`, `aa` (Acceptable Ads)
  - `action` is either:
      - `add`, `clear` (remove all), `list`, `remove` valid for `domains`,
        `filters` and `subscriptions`
      - `enable`, `disable` or `state` valid for `aa` and `adblock`
  - `payload` is url encoded string required for action `add` and `remove`.
  When adding or removing filter/domain/subscription one can encode several
  entries splitting them by a new line character.

  An example:
  Call `adblock.test.data/filters/add/%2FadsPlugin%2F%2A%0A%2Fadsponsor.`
  Then calling `adblock.test.data/filters/list` returns:

    OK

    /adsPlugin/*
    /adsponsor.
)";

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

std::string GetPayload(const std::string& input) {
  static const char kPayloadParam[] = "payload=";
  if (base::StartsWith(input, kPayloadParam)) {
    return input.substr(sizeof(kPayloadParam) - 1);
  }
  return "";
}

std::vector<std::string> GetCommandElements(const GURL& url) {
  std::vector<std::string> command_elements;
  auto path = url.path();
  if (path.length() > 1) {
    path.erase(0, 1);
    command_elements = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                         base::SPLIT_WANT_NONEMPTY);
    if (command_elements.size() == 1) {
      // Check old format
      if (path == kActionAdd || path == kActionRemove) {
        auto payload = GetPayload(url.query());
        if (payload.length() > 0) {
          command_elements = {kTopicFilters,
                              path == kActionAdd ? kActionAdd : kActionRemove,
                              payload};
        }
      }
    }
  }
  return command_elements;
}

adblock::FilteringConfiguration* GetAdblockConfiguration(
    SubscriptionService* subscription_service) {
  auto* adblock_configuration = subscription_service->GetFilteringConfiguration(
      adblock::kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration)
      << "Action called for non existing \"adblock\" configuration";
  return adblock_configuration;
}

void Clear(SubscriptionService* subscription_service,
           std::vector<std::string> (FilteringConfiguration::*getter)() const,
           void (FilteringConfiguration::*action)(const std::string&)) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  for (const auto& item : (adblock_configuration->*getter)()) {
    (adblock_configuration->*action)(item);
  }
}

std::vector<std::string> List(
    SubscriptionService* subscription_service,
    std::vector<std::string> (FilteringConfiguration::*getter)() const) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  return (adblock_configuration->*getter)();
}

void AddOrRemove(SubscriptionService* subscription_service,
                 void (FilteringConfiguration::*action)(const std::string&),
                 std::string& items) {
  auto items_list = base::SplitString(items, payload_delimiter,
                                      base::WhitespaceHandling::TRIM_WHITESPACE,
                                      base::SplitResult::SPLIT_WANT_NONEMPTY);
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  for (const auto& item : items_list) {
    (adblock_configuration->*action)(item);
  }
}

void ClearSubscriptions(SubscriptionService* subscription_service) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  for (const auto& subscription : adblock_configuration->GetFilterLists()) {
    adblock_configuration->RemoveFilterList(subscription);
  }
}

std::vector<std::string> ListSubscriptions(
    SubscriptionService* subscription_service) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  std::vector<std::string> subscriptions;
  base::ranges::transform(
      adblock_configuration->GetFilterLists(),
      std::back_inserter(subscriptions),
      [](const auto& subscription) { return subscription.spec(); });
  return subscriptions;
}

void AddOrRemoveSubscription(
    SubscriptionService* subscription_service,
    void (FilteringConfiguration::*action)(const GURL&),
    std::string& items) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  auto items_list = base::SplitString(items, payload_delimiter,
                                      base::WhitespaceHandling::TRIM_WHITESPACE,
                                      base::SplitResult::SPLIT_WANT_NONEMPTY);
  for (const auto& item : items_list) {
    (adblock_configuration->*action)(GURL{item});
  }
}

bool IsAdblockEnabled(SubscriptionService* subscription_service) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  return adblock_configuration->IsEnabled();
}

void SetAdblockEnabled(SubscriptionService* subscription_service,
                       bool enabled) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  adblock_configuration->SetEnabled(enabled);
}

bool IsAAEnabled(SubscriptionService* subscription_service) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  return base::ranges::any_of(
      adblock_configuration->GetFilterLists(),
      [&](const auto& url) { return url == AcceptableAdsUrl(); });
}

void SetAAEnabled(SubscriptionService* subscription_service, bool enabled) {
  auto* adblock_configuration = GetAdblockConfiguration(subscription_service);
  enabled ? adblock_configuration->AddFilterList(AcceptableAdsUrl())
          : adblock_configuration->RemoveFilterList(AcceptableAdsUrl());
}

}  // namespace

// static
const char AdblockURLLoaderFactoryForTest::kAdblockDebugDataHostName[] =
    "adblock.test.data";

AdblockURLLoaderFactoryForTest::AdblockURLLoaderFactoryForTest(
    AdblockURLLoaderFactoryConfig config,
    content::GlobalRenderFrameHostId host_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    std::string user_agent_string,
    DisconnectCallback on_disconnect,
    SubscriptionService* subscription_service)
    : AdblockURLLoaderFactory(std::move(config),
                              host_id,
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
  if (request.url.host() != kAdblockDebugDataHostName) {
    DLOG(WARNING)
        << "[eyeo] AdblockURLLoaderFactoryForTest got unexpected url: "
        << request.url;
    AdblockURLLoaderFactory::CreateLoaderAndStart(
        std::move(loader), request_id, options, request, std::move(client),
        traffic_annotation);
    return;
  }
  VLOG(2) << "[eyeo] AdblockURLLoaderFactoryForTest handles: " << request.url;
  std::string response_body = HandleCommand(request.url);
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
  uint32_t write_size = static_cast<uint32_t>(response_body.size());
  producer->WriteData(response_body.c_str(), &write_size,
                      MOJO_WRITE_DATA_FLAG_NONE);
  client_remote->OnReceiveResponse(std::move(response_head),
                                   std::move(consumer), absl::nullopt);
  network::URLLoaderCompletionStatus status;
  status.error_code = net::OK;
  status.decoded_body_length = write_size;
  client_remote->OnComplete(status);
}

std::string AdblockURLLoaderFactoryForTest::HandleCommand(
    const GURL& url) const {
  auto command_elements = GetCommandElements(url);
  if (command_elements.size() == 1 && command_elements[0] == kTopicHelp) {
    return kHelp;
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
          AddOrRemoveSubscription(subscription_service_, action_ptr, payload);
          return kResponseOk;
        }
      } else if (action == kActionClear) {
        ClearSubscriptions(subscription_service_);
        return kResponseOk;
      } else if (action == kActionList) {
        std::string response = kResponseOk;
        auto subscriptions = ListSubscriptions(subscription_service_);
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
          AddOrRemove(subscription_service_, action_ptr, payload);
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
          Clear(subscription_service_, getter, deleter);
          return kResponseOk;
        } else if (getter) {
          std::string response = kResponseOk;
          auto items = List(subscription_service_, getter);
          if (!items.empty()) {
            response += "\n\n";
            for (const auto& item : items) {
              response += item + "\n";
            }
          }
          return response;
        }
      }
    } else if (topic == kTopicAdblock || topic == kTopicAcceptableAds) {
      if (action == kActionState) {
        std::string response = kResponseOk;
        response += "\n\n";
        bool enabled = topic == kTopicAdblock
                           ? IsAdblockEnabled(subscription_service_)
                           : IsAAEnabled(subscription_service_);
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
        if (topic == kTopicAdblock) {
          SetAdblockEnabled(subscription_service_, value.value());
        } else {
          SetAAEnabled(subscription_service_, value.value());
        }
        return kResponseOk;
      }
    }
  }
  return kResponseInvalidCommand;
}

}  // namespace adblock
