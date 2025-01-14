/* This file is part of eyeo Chromium SDK,
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

#include "components/adblock/core/activeping_telemetry_topic_provider.h"

#include <string_view>

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/app_info.h"
#include "components/adblock/core/subscription/subscription_config.h"

namespace adblock {
namespace {
int g_https_port_for_testing = 0;
std::optional<base::TimeDelta> g_time_delta_for_testing;

GURL GetUrl() {
  GURL url(EYEO_TELEMETRY_SERVER_URL);
  if (!g_https_port_for_testing) {
    return url;
  }
  GURL::Replacements replacements;
  const std::string port_str = base::NumberToString(g_https_port_for_testing);
  replacements.SetPortStr(port_str);
  return url.ReplaceComponents(replacements);
}

base::TimeDelta GetNormalPingInterval() {
  static base::TimeDelta kNormalPingInterval =
      g_time_delta_for_testing ? g_time_delta_for_testing.value()
                               : base::Hours(12);
  return kNormalPingInterval;
}

base::TimeDelta GetRetryPingInterval() {
  static base::TimeDelta kRetryPingInterval =
      g_time_delta_for_testing ? g_time_delta_for_testing.value()
                               : base::Hours(1);
  return kRetryPingInterval;
}

void AppendStringIfPresent(PrefService* pref_service,
                           const std::string& pref_name,
                           std::string_view payload_key,
                           base::Value::Dict& payload) {
  auto str = pref_service->GetString(pref_name);
  if (!str.empty()) {
    payload.Set(payload_key, std::move(str));
  }
}

std::string FormatNextRequestTime(base::Time time) {
  if (time.is_null()) {
    return "[Unset]";
  }
  return base::UTF16ToUTF8(base::TimeFormatFriendlyDateAndTime(time));
}
}  // namespace

ActivepingTelemetryTopicProvider::ActivepingTelemetryTopicProvider(
    PrefService* pref_service,
    SubscriptionService* subscription_service,
    const GURL& base_url,
    const std::string& auth_token,
    std::unique_ptr<StatsPayloadProvider> stats_payload_provider)
    : pref_service_(pref_service),
      subscription_service_(subscription_service),
      base_url_(base_url),
      auth_token_(auth_token),
      stats_payload_provider_(std::move(stats_payload_provider)) {}

ActivepingTelemetryTopicProvider::~ActivepingTelemetryTopicProvider() = default;

// static
GURL ActivepingTelemetryTopicProvider::DefaultBaseUrl() {
#if !defined(EYEO_TELEMETRY_CLIENT_ID)
  LOG(WARNING)
      << "[eyeo] Using default Telemetry server since a Telemetry client ID "
         "was "
         "not provided. Users will not be counted correctly by eyeo. Please "
         "set an ID via \"eyeo_telemetry_client_id\" gn argument.";
#endif
  return GetUrl();
}

// static
std::string ActivepingTelemetryTopicProvider::DefaultAuthToken() {
#if defined(EYEO_TELEMETRY_ACTIVEPING_AUTH_TOKEN)
  DVLOG(1) << "[eyeo] Using " << EYEO_TELEMETRY_ACTIVEPING_AUTH_TOKEN
           << " as Telemetry authentication token";
  return EYEO_TELEMETRY_ACTIVEPING_AUTH_TOKEN;
#else
  LOG(WARNING)
      << "[eyeo] No Telemetry authentication token defined. Users will "
         "not be counted correctly by eyeo. Please set a token via "
         "\"eyeo_telemetry_activeping_auth_token\" gn argument.";
  return "";
#endif
}

GURL ActivepingTelemetryTopicProvider::GetEndpointURL() const {
  return base_url_.Resolve("/topic/eyeochromium_activeping/version/2");
}

std::string ActivepingTelemetryTopicProvider::GetAuthToken() const {
  return auth_token_;
}

void ActivepingTelemetryTopicProvider::GetPayload(
    PayloadCallback callback) const {
  std::string serialized;
  // The only way JSONWriter::Write() can return fail is then the Value
  // contains lists or dicts that are too deep (200 levels). We just built the
  // payload and root objects here, they should be really shallow.
  CHECK(base::JSONWriter::Write(GetPayloadInternal(), &serialized));
  std::move(callback).Run(std::move(serialized));
}

base::Time ActivepingTelemetryTopicProvider::GetTimeOfNextRequest() const {
  const auto next_ping_time =
      pref_service_->GetTime(common::prefs::kTelemetryNextPingTime);
  // Next ping time may be unset if this is a first run. Next request should
  // happen ASAP.
  if (next_ping_time.is_null()) {
    return base::Time::Now();
  }

  return next_ping_time;
}

void ActivepingTelemetryTopicProvider::ParseResponse(
    std::unique_ptr<std::string> response_content) {
  if (!response_content) {
    VLOG(1) << "[eyeo] Telemetry ping failed, no response from server";
    ScheduleNextPing(GetRetryPingInterval());
    return;
  }

  VLOG(1) << "[eyeo] Response from Telemetry server: " << *response_content;
  auto parsed = base::JSONReader::ReadDict(*response_content);
  if (!parsed) {
    VLOG(1)
        << "[eyeo] Telemetry ping failed, response could not be parsed as JSON";
    ScheduleNextPing(GetRetryPingInterval());
    return;
  }

  auto* error_message = parsed->FindString("error");
  if (error_message) {
    VLOG(1) << "[eyeo] Telemetry ping failed, error message: "
            << *error_message;
    ScheduleNextPing(GetRetryPingInterval());
    return;
  }

  // For legacy reasons, "ping_response_time" is sent to us as "token". This
  // should be the server time of when the ping was handled, possibly truncated
  // for anonymity. We don't parse it or interpret it, just send it back with
  // next ping.
  auto* ping_response_time = parsed->FindString("token");
  if (!ping_response_time) {
    VLOG(1) << "[eyeo] Telemetry ping failed, response did not contain a last "
               "ping / token value";
    ScheduleNextPing(GetRetryPingInterval());
    return;
  }

  VLOG(1) << "[eyeo] Telemetry ping succeeded";
  ScheduleNextPing(GetNormalPingInterval());
  UpdatePrefs(*ping_response_time);
}

void ActivepingTelemetryTopicProvider::FetchDebugInfo(
    DebugInfoCallback callback) const {
  base::Value::Dict debug_info;
  debug_info.Set("endpoint_url", GetEndpointURL().spec());
  debug_info.Set("payload", GetPayloadInternal());
  debug_info.Set("first_ping",
                 pref_service_->GetString(
                     adblock::common::prefs::kTelemetryFirstPingTime));
  debug_info.Set("time_of_next_request",
                 FormatNextRequestTime(GetTimeOfNextRequest()));
  debug_info.Set(
      "last_ping",
      pref_service_->GetString(adblock::common::prefs::kTelemetryLastPingTime));
  debug_info.Set("previous_last_ping",
                 pref_service_->GetString(
                     adblock::common::prefs::kTelemetryPreviousLastPingTime));

  std::string serialized;
  // The only way JSONWriter::Write() can return fail is then the Value
  // contains lists or dicts that are too deep (200 levels). We just built the
  // payload and root objects here, they should be really shallow.
  CHECK(base::JSONWriter::WriteWithOptions(
      debug_info, base::JsonOptions::OPTIONS_PRETTY_PRINT, &serialized));
  std::move(callback).Run(std::move(serialized));
}

void ActivepingTelemetryTopicProvider::ScheduleNextPing(base::TimeDelta delay) {
  pref_service_->SetTime(common::prefs::kTelemetryNextPingTime,
                         base::Time::Now() + delay);
  if (stats_payload_provider_ && delay == GetNormalPingInterval()) {
    stats_payload_provider_->ResetStats();
  }
}

void ActivepingTelemetryTopicProvider::UpdatePrefs(
    const std::string& ping_response_time) {
  // First ping is only set once per client.
  if (pref_service_->GetString(common::prefs::kTelemetryFirstPingTime)
          .empty()) {
    pref_service_->SetString(common::prefs::kTelemetryFirstPingTime,
                             ping_response_time);
  }
  // Previous-to-last becomes last, last becomes current.
  pref_service_->SetString(
      common::prefs::kTelemetryPreviousLastPingTime,
      pref_service_->GetString(common::prefs::kTelemetryLastPingTime));
  pref_service_->SetString(common::prefs::kTelemetryLastPingTime,
                           ping_response_time);
  // Generate a new random tag that wil be sent along with ping times in the
  // next request.
  const auto tag = base::Uuid::GenerateRandomV4();
  pref_service_->SetString(common::prefs::kTelemetryLastPingTag,
                           tag.AsLowercaseString());
}

base::Value ActivepingTelemetryTopicProvider::GetPayloadInternal() const {
  base::Value::Dict payload;
  bool aa_enabled = false;
  auto* adblock_configuration =
      subscription_service_->GetFilteringConfiguration(
          kAdblockFilteringConfigurationName);
  if (adblock_configuration) {
    aa_enabled = base::ranges::any_of(
        adblock_configuration->GetFilterLists(),
        [&](const auto& url) { return url == AcceptableAdsUrl(); });
  }
  payload.Set("addon_name", "eyeo-chromium-sdk");
  payload.Set("addon_version", "2.0.0");
  payload.Set("application", AppInfo::Get().name);
  payload.Set("application_version", AppInfo::Get().version);
  payload.Set("aa_active", aa_enabled);
  payload.Set("platform", base::SysInfo::OperatingSystemName());
  payload.Set("platform_version", base::SysInfo::OperatingSystemVersion());
  if (stats_payload_provider_) {
    payload.Merge(stats_payload_provider_->GetPayload());
  }
  // Server requires the following parameters to either have a correct,
  // non-empty value, or not be present at all. We shall not send empty strings.
  AppendStringIfPresent(pref_service_, common::prefs::kTelemetryLastPingTag,
                        "last_ping_tag", payload);
  AppendStringIfPresent(pref_service_, common::prefs::kTelemetryFirstPingTime,
                        "first_ping", payload);
  AppendStringIfPresent(pref_service_, common::prefs::kTelemetryLastPingTime,
                        "last_ping", payload);
  AppendStringIfPresent(pref_service_,
                        common::prefs::kTelemetryPreviousLastPingTime,
                        "previous_last_ping", payload);

  base::Value::Dict root;
  root.Set("payload", std::move(payload));
  return base::Value(std::move(root));
}

// static
void ActivepingTelemetryTopicProvider::SetHttpsPortForTesting(
    int https_port_for_testing) {
  g_https_port_for_testing = https_port_for_testing;
}

// static
void ActivepingTelemetryTopicProvider::SetIntervalsForTesting(
    base::TimeDelta time_delta) {
  g_time_delta_for_testing = time_delta;
}

}  // namespace adblock
