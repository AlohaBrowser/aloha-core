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

#ifndef COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace adblock {
/**
 * @brief Sends periodic pings to eyeo in order to count active users. Executed
 * from Browser process UI main thread.
 */
class AdblockTelemetryService
    : public KeyedService,
      public FilteringConfiguration::Observer,
      public SubscriptionService::SubscriptionObserver {
 public:
  // Provides data and behavior relevant for a Telemetry "topic". A topic could
  // be "counting users" or "reporting filter list hits" for example.
  class TopicProvider {
   public:
    using PayloadCallback = base::OnceCallback<void(std::string payload)>;
    using DebugInfoCallback = base::OnceCallback<void(std::string payload)>;
    virtual ~TopicProvider() = default;
    // Endpoint URL on the Telemetry server onto which requests should be sent.
    virtual GURL GetEndpointURL() const = 0;
    // Authorization bearer token for the endpoint defined by GetEndpointURL().
    virtual std::string GetAuthToken() const = 0;
    // Data uploaded with the request, should be valid for the schema
    // present on the server. Async to allow querying asynchronous data sources.
    virtual void GetPayload(PayloadCallback callback) const = 0;
    // Returns the desired time when AdblockTelemetryService should make the
    // next network request.
    virtual base::Time GetTimeOfNextRequest() const = 0;
    // Parses the response returned by the Telemetry server. |response_content|
    // may be null. Implementation is free to implement a "retry" in case of
    // response errors via GetTimeToNextRequest().
    virtual void ParseResponse(
        std::unique_ptr<std::string> response_content) = 0;
    // Gets debugging info to be logged on chrome://adblock-internals. Do not
    // put any secrets here (tokens, api keys). Asynchronous to allow reusing
    // the async logic of GetPayload, if needed.
    virtual void FetchDebugInfo(DebugInfoCallback callback) const = 0;
  };
  AdblockTelemetryService(
      SubscriptionService* subscription_service_,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      base::TimeDelta initial_delay,
      base::TimeDelta check_interval);
  ~AdblockTelemetryService() override;
  using TopicProvidersDebugInfoCallback =
      base::OnceCallback<void(std::vector<std::string>)>;

  // Add all required topic providers before calling Start().
  void AddTopicProvider(std::unique_ptr<TopicProvider> topic_provider);

  // Starts periodic Telemetry requests, provided ad-blocking is enabled.
  // If ad blocking is disabled, the schedule will instead start when
  // ad blocking becomes enabled.
  void Start();

  // KeyedService:
  void Shutdown() override;

  // FilteringConfiguration::Observer
  void OnEnabledStateChanged(FilteringConfiguration* config) override;

  // Collects debug information from all topic providers. Runs |callback| once
  // all topic providers have provided their info.
  void GetTopicProvidersDebugInfo(
      TopicProvidersDebugInfoCallback callback) const;
  // SubscriptionService::SubscriptionObserver
  void OnFilteringConfigurationInstalled(
      FilteringConfiguration* config) override;
  void OnFilteringConfigurationUninstalled(
      std::string_view config_name) override;

 private:
  void RunPeriodicCheck();

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<SubscriptionService> subscription_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::TimeDelta initial_delay_;
  base::TimeDelta check_interval_;

  class Conversation;
  std::vector<std::unique_ptr<Conversation>> ongoing_conversations_;
  base::OneShotTimer timer_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_
