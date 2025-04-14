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

#include "content/browser/devtools/protocol/eyeo_handler.h"

#include "base/types/strong_alias.h"
#include "components/adblock/core/adblock_telemetry_service.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_keyed_service_ptr_holder.h"
#include "components/adblock/core/session_stats.h"
#include "components/adblock/core/subscription/subscription_service_impl.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace content {
namespace protocol {

namespace {

using ConfigurationError =
    base::StrongAlias<class ConfigurationErrorTag, std::string>;

adblock::SubscriptionService* GetSubscriptionService(RenderFrameHostImpl* rfh) {
  auto* context = rfh->GetProcess()->GetBrowserContext();
  return adblock::KeyedServicePtrHolder<adblock::SubscriptionService>::Get(
      context, adblock::kSubscriptionServiceUserDataKey);
}

absl::variant<adblock::FilteringConfiguration*, ConfigurationError>
MaybeGetConfiguration(RenderFrameHostImpl* rfh,
                      const std::optional<std::string>& configuration) {
  auto* subscription_service = GetSubscriptionService(rfh);
  if (!subscription_service) {
    LOG(ERROR) << "[eyeo] EyeoHandler: Missing SubscriptionService!";
    return ConfigurationError("Missing SubscriptionService!");
  }
  std::string configuration_name = configuration.value_or("adblock");
  auto* configuration_ptr =
      subscription_service->GetFilteringConfiguration(configuration_name);
  if (!configuration_ptr) {
    LOG(INFO) << "[eyeo] EyeoHandler: Missing configuration "
              << configuration_name;
    return ConfigurationError("Missing configuration " + configuration_name);
  }
  return configuration_ptr;
}

std::vector<adblock::FilteringConfiguration*> GetConfigurationsInternal(
    RenderFrameHostImpl* rfh) {
  auto* subscription_service = GetSubscriptionService(rfh);
  if (!subscription_service) {
    LOG(ERROR) << "[eyeo] EyeoHandler: Missing SubscriptionService!";
    return {};
  }
  return subscription_service->GetInstalledFilteringConfigurations();
}

}  // namespace

EyeoHandler::EyeoHandler()
    : DevToolsDomainHandler(Eyeo::Metainfo::domainName) {}
EyeoHandler::~EyeoHandler() = default;

// static
std::vector<EyeoHandler*> EyeoHandler::ForAgentHost(
    DevToolsAgentHostImpl* host) {
  return host->HandlersByName<EyeoHandler>(Eyeo::Metainfo::domainName);
}

void EyeoHandler::SetRenderer(int process_host_id,
                              RenderFrameHostImpl* frame_host) {
  rfh_ = frame_host;
}

void EyeoHandler::Wire(UberDispatcher* dispatcher) {
#if BUILDFLAG(IS_ANDROID) || defined(EYEO_EXTEND_CHROME_DEVTOOLS_PROTOCOL)
  frontend_ = std::make_unique<Eyeo::Frontend>(dispatcher->channel());
  Eyeo::Dispatcher::wire(dispatcher, this);
#endif
}

Response EyeoHandler::Enable() {
  if (enabled_) {
    return Response::Success();
  }
  enabled_ = true;

  for (auto* configuration : GetConfigurationsInternal(rfh_)) {
    configuration->AddObserver(this);
  }

  auto* classification_runner =
      adblock::KeyedServicePtrHolder<adblock::ResourceClassificationRunner>::
          Get(rfh_->GetProcess()->GetBrowserContext(),
              adblock::kResourceClassificationRunnerUserDataKey);
  DCHECK(classification_runner);
  classification_runner->AddObserver(this);

  return Response::Success();
}

Response EyeoHandler::Disable() {
  if (!enabled_) {
    return Response::Success();
  }
  enabled_ = false;

  for (auto* configuration : GetConfigurationsInternal(rfh_)) {
    configuration->RemoveObserver(this);
  }

  auto* classification_runner =
      adblock::KeyedServicePtrHolder<adblock::ResourceClassificationRunner>::
          Get(rfh_->GetProcess()->GetBrowserContext(),
              adblock::kResourceClassificationRunnerUserDataKey);
  DCHECK(classification_runner);
  classification_runner->RemoveObserver(this);

  return Response::Success();
}

Response EyeoHandler::GetConfigurations(
    std::unique_ptr<protocol::Array<std::string>>* lists) {
  *lists = std::make_unique<protocol::Array<std::string>>();
  for (const auto& configuration : GetConfigurationsInternal(rfh_)) {
    (*lists)->emplace_back(
        configuration->GetName() +
        (configuration->IsEnabled() ? ": enabled" : ": disabled"));
  }
  return Response::Success();
}

Response EyeoHandler::GetDownloadStats(
    std::optional<std::string> configuration,
    std::unique_ptr<protocol::Array<std::string>>* result) {
  auto* subscription_service = GetSubscriptionService(rfh_);
  if (!subscription_service) {
    return Response::ServerError("Missing SubscriptionService!");
  }

  *result = std::make_unique<protocol::Array<std::string>>();
  auto download_stats =
      subscription_service->GetDownloadStats(configuration.value_or("adblock"));
  if (download_stats.empty()) {
    return Response::ServerError("Invalid configuration name: " +
                                 configuration.value_or("adblock"));
  }
  for (const auto& d_s : download_stats) {
    (*result)->emplace_back(
        "{ url : " + d_s.first.spec() +
        ", counters : { success : " + std::to_string(d_s.second.success_count) +
        ", errors : " + std::to_string(d_s.second.error_count) + "} }");
  }
  return Response::Success();
}

Response EyeoHandler::GetSessionStats(
    std::optional<std::string> configuration,
    std::unique_ptr<protocol::Array<std::string>>* result) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  auto filter_lists = configuration_ptr->GetFilterLists();

  auto* context = rfh_->GetProcess()->GetBrowserContext();
  auto* session_stats_service =
      adblock::KeyedServicePtrHolder<adblock::SessionStats>::Get(
          context, adblock::kSessionStatsServiceUserDataKey);
  if (!session_stats_service) {
    LOG(ERROR) << "[eyeo] EyeoHandler: Missing SessionStatsService!";
    return Response::ServerError("Missing SessionStatsService!");
  }
  auto allowed_map = session_stats_service->GetSessionAllowedResourcesCount();
  auto blocked_map = session_stats_service->GetSessionBlockedResourcesCount();

  *result = std::make_unique<protocol::Array<std::string>>();
  for (const auto& url : filter_lists) {
    std::string url_str = url.spec();
    std::string allowed_count =
        allowed_map.contains(url) ? std::to_string(allowed_map[url]) : "0";
    std::string blocked_count =
        blocked_map.contains(url) ? std::to_string(blocked_map[url]) : "0";
    (*result)->emplace_back("{ url : " + url_str +
                            ", counters : { allowed : " + allowed_count +
                            ", blocked : " + blocked_count + "} }");
  }

  return Response::Success();
}

Response EyeoHandler::GetFilterLists(
    std::optional<std::string> configuration,
    std::unique_ptr<protocol::Array<std::string>>* lists) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  *lists = std::make_unique<protocol::Array<std::string>>();
  auto filter_lists = configuration_ptr->GetFilterLists();
  for (const auto& url : filter_lists) {
    (*lists)->emplace_back(url.spec());
  }
  return Response::Success();
}

Response EyeoHandler::AddFilterList(std::optional<std::string> configuration,
                                    const std::string& url) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->AddFilterList(GURL{url});
  return Response::Success();
}

Response EyeoHandler::RemoveFilterList(std::optional<std::string> configuration,
                                       const std::string& url) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->RemoveFilterList(GURL{url});
  return Response::Success();
}

Response EyeoHandler::GetCustomFilters(
    std::optional<std::string> configuration,
    std::unique_ptr<protocol::Array<std::string>>* lists) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  *lists = std::make_unique<protocol::Array<std::string>>();
  auto custom_filters = configuration_ptr->GetCustomFilters();
  for (const auto& filter : custom_filters) {
    (*lists)->emplace_back(filter);
  }
  return Response::Success();
}

Response EyeoHandler::AddCustomFilter(std::optional<std::string> configuration,
                                      const std::string& filter) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->AddCustomFilter(filter);
  return Response::Success();
}

Response EyeoHandler::RemoveCustomFilter(
    std::optional<std::string> configuration,
    const std::string& filter) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->RemoveCustomFilter(filter);
  return Response::Success();
}

Response EyeoHandler::GetAllowedDomains(
    std::optional<std::string> configuration,
    std::unique_ptr<protocol::Array<std::string>>* domains) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  *domains = std::make_unique<protocol::Array<std::string>>();
  auto allowed_domains = configuration_ptr->GetAllowedDomains();
  for (const auto& domain : allowed_domains) {
    (*domains)->emplace_back(domain);
  }
  return Response::Success();
}

Response EyeoHandler::AddAllowedDomain(std::optional<std::string> configuration,
                                       const std::string& domain) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->AddAllowedDomain(domain);
  return Response::Success();
}

Response EyeoHandler::RemoveAllowedDomain(
    std::optional<std::string> configuration,
    const std::string& domain) {
  auto configuration_or_error = MaybeGetConfiguration(rfh_, configuration);
  if (absl::holds_alternative<ConfigurationError>(configuration_or_error)) {
    return Response::ServerError(
        *absl::get<ConfigurationError>(std::move(configuration_or_error)));
  }
  auto* configuration_ptr =
      absl::get<adblock::FilteringConfiguration*>(configuration_or_error);
  configuration_ptr->RemoveAllowedDomain(domain);
  return Response::Success();
}

Response EyeoHandler::GetTelemetryDebugInfo(protocol::String* result) {
  auto* context = rfh_->GetProcess()->GetBrowserContext();
  auto* telemetry_service =
      adblock::KeyedServicePtrHolder<adblock::AdblockTelemetryService>::Get(
          context, adblock::kAdblockTelemetryServiceUserDataKey);
  if (!telemetry_service) {
    LOG(ERROR) << "[eyeo] EyeoHandler: Missing AdblockTelemetryService!";
    return Response::ServerError("Missing AdblockTelemetryService!");
  }

  for (auto& topic_provider_debug_info :
       telemetry_service->GetTopicProvidersDebugInfo()) {
    *result += topic_provider_debug_info + ", ";
  }

  return Response::Success();
}

void EyeoHandler::OnFilterListsChanged(
    adblock::FilteringConfiguration* config) {
  if (!enabled_ || !frontend_) {
    return;
  }
  std::unique_ptr<protocol::Array<std::string>> urls =
      std::make_unique<protocol::Array<std::string>>();
  for (const auto& url : config->GetFilterLists()) {
    urls->emplace_back(url.spec());
  }
  frontend_->FilterListsChanged(config->GetName(), std::move(urls));
}

void EyeoHandler::OnCustomFiltersChanged(
    adblock::FilteringConfiguration* config) {
  if (!enabled_ || !frontend_) {
    return;
  }
  std::unique_ptr<protocol::Array<std::string>> filters =
      std::make_unique<protocol::Array<std::string>>();
  for (const auto& filter : config->GetCustomFilters()) {
    filters->emplace_back(filter);
  }
  frontend_->CustomFiltersChanged(config->GetName(), std::move(filters));
}

void EyeoHandler::OnPageAllowed(const GURL& url,
                                content::RenderFrameHost* render_frame_host,
                                const GURL& subscription,
                                const std::string& configuration_name) {
  if (!enabled_ || render_frame_host != rfh_ || render_frame_host == nullptr ||
      !frontend_) {
    return;
  }
  frontend_->PageAllowed(subscription.spec(), url.spec());
}

void EyeoHandler::OnRequestMatched(const GURL& url,
                                   adblock::FilterMatchResult match_result,
                                   const std::vector<GURL>& parent_frame_urls,
                                   adblock::ContentType content_type,
                                   content::RenderFrameHost* render_frame_host,
                                   const GURL& subscription,
                                   const std::string& configuration_name) {
  OnMatchedInternal(url, match_result, content_type, render_frame_host,
                    subscription);
}

void EyeoHandler::OnPopupMatched(const GURL& url,
                                 adblock::FilterMatchResult match_result,
                                 const GURL& opener_url,
                                 content::RenderFrameHost* render_frame_host,
                                 const GURL& subscription,
                                 const std::string& configuration_name) {
  OnMatchedInternal(url, match_result, adblock::ContentType::Other,
                    render_frame_host, subscription);
}

void EyeoHandler::OnMatchedInternal(const GURL& url,
                                    adblock::FilterMatchResult match_result,
                                    adblock::ContentType content_type,
                                    content::RenderFrameHost* render_frame_host,
                                    const GURL& subscription) {
  if (!enabled_ || render_frame_host != rfh_ || render_frame_host == nullptr ||
      !frontend_) {
    return;
  }

  if (match_result == adblock::FilterMatchResult::kAllowRule) {
    frontend_->RequestAllowed(subscription.spec(), url.spec());
  } else {
    DCHECK(match_result == adblock::FilterMatchResult::kBlockRule);
    frontend_->RequestBlocked(subscription.spec(), url.spec());
  }
}

}  // namespace protocol
}  // namespace content
