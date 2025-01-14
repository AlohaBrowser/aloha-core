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

#ifndef COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_
#define COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "components/adblock/core/adblock_telemetry_service.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_service.h"

namespace adblock {

// Telemetry topic provider that uploads user-counting data for periodic pings.
// Provides the following data in Payload:
// - Last ping time, previous-to-last ping time, first ping time
// - Unique, non-persistent tag for disambiguating pings made by clients in
//   the same day
// - Whether Acceptable Ads is enabled
// - Application name & version, platform name & version
// Note: Provides no user-identifiable information, no persistent tracking
// data (ie. no traceable UUID) and no information about user actions.
class ActivepingTelemetryTopicProvider final
    : public AdblockTelemetryService::TopicProvider {
 public:
  // Provides additional statistics payload for ActivepingTelemetryTopicProvider
  class StatsPayloadProvider {
   public:
    virtual ~StatsPayloadProvider() = default;
    virtual base::Value::Dict GetPayload() const = 0;
    virtual void ResetStats() = 0;
  };
  ActivepingTelemetryTopicProvider(
      PrefService* pref_service,
      SubscriptionService* subscription_service,
      const GURL& base_url,
      const std::string& auth_token,
      std::unique_ptr<StatsPayloadProvider> stats_payload_provider);
  ~ActivepingTelemetryTopicProvider() final;

  static GURL DefaultBaseUrl();
  static std::string DefaultAuthToken();

  GURL GetEndpointURL() const final;
  std::string GetAuthToken() const final;
  void GetPayload(PayloadCallback callback) const final;

  // Normally 12 hours since last ping, 1 hour in case of retries.
  base::Time GetTimeOfNextRequest() const final;

  // Attempts to parse "token" (an opaque server description of last ping time)
  // from |response_content|.
  void ParseResponse(std::unique_ptr<std::string> response_content) final;

  void FetchDebugInfo(DebugInfoCallback callback) const final;

  // Sets the port used by the embedded http server required for browser tests.
  // Must be called before the first call to DefaultBaseUrl().
  static void SetHttpsPortForTesting(int https_port_for_testing);

  // Sets the internal timing for sending pings required for browser tests.
  // Must be called before AdblockTelemetryService::Start().
  static void SetIntervalsForTesting(base::TimeDelta time_delta);

 private:
  void ScheduleNextPing(base::TimeDelta delay);
  void UpdatePrefs(const std::string& ping_response_time);
  base::Value GetPayloadInternal() const;

  raw_ptr<PrefService> pref_service_;
  raw_ptr<SubscriptionService> subscription_service_;
  const GURL base_url_;
  const std::string auth_token_;
  std::unique_ptr<StatsPayloadProvider> stats_payload_provider_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_
