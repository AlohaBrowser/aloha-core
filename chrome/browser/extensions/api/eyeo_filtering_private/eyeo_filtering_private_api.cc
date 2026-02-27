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

#include "chrome/browser/extensions/api/eyeo_filtering_private/eyeo_filtering_private_api.h"

#include "base/containers/flat_map.h"
#include "base/i18n/time_formatting.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/tabs.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/session_stats_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/session_stats.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
namespace extensions {

namespace {

constexpr char kConfigurationMissing[] =
    "Configuration with name '%s' does not exist!";

adblock::FilteringConfiguration* FindFilteringConfiguration(
    content::BrowserContext* context,
    const std::string& config_name) {
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(context);
  const auto installed_configurations =
      subscription_service->GetInstalledFilteringConfigurations();
  auto configuration_it =
      std::ranges::find(installed_configurations, config_name,
                        &adblock::FilteringConfiguration::GetName);
  return configuration_it != installed_configurations.end() ? *configuration_it
                                                            : nullptr;
}

enum class SubscriptionAction { kInstall, kUninstall };

std::string RunSubscriptionAction(
    adblock::FilteringConfiguration* configuration,
    SubscriptionAction action,
    const GURL& url) {
  if (!url.is_valid()) {
    return "Invalid URL";
  }
  switch (action) {
    case SubscriptionAction::kInstall:
      configuration->AddFilterList(url);
      break;
    case SubscriptionAction::kUninstall:
      configuration->RemoveFilterList(url);
      break;
    default:
      NOTREACHED();
  }

  return {};
}

std::vector<api::eyeo_filtering_private::SessionStatsEntry> CopySessionsStats(
    const std::map<GURL, long>& source) {
  std::vector<api::eyeo_filtering_private::SessionStatsEntry> result;
  for (auto& entry : source) {
    api::eyeo_filtering_private::SessionStatsEntry js_entry;
    js_entry.url = entry.first.spec();
    js_entry.count = entry.second;
    result.emplace_back(std::move(js_entry));
  }
  return result;
}

std::string SubscriptionInstallationStateToString(
    adblock::Subscription::InstallationState state) {
  using State = adblock::Subscription::InstallationState;
  switch (state) {
    case State::Installed:
      return "Installed";
    case State::AutoInstalled:
      return "AutoInstalled";
    case State::Preloaded:
      return "Preloaded";
    case State::Installing:
      return "Installing";
    case State::Unknown:
      return "Unknown";
  }
  return "";
}

std::vector<api::eyeo_filtering_private::Subscription> CopySubscriptions(
    const std::vector<scoped_refptr<adblock::Subscription>>
        current_subscriptions) {
  std::vector<api::eyeo_filtering_private::Subscription> result;
  for (auto& sub : current_subscriptions) {
    api::eyeo_filtering_private::Subscription js_sub;
    js_sub.url = sub->GetSourceUrl().spec();
    js_sub.title = sub->GetTitle();
    js_sub.current_version = sub->GetCurrentVersion();
    js_sub.installation_state =
        SubscriptionInstallationStateToString(sub->GetInstallationState());
    const auto installation_time = sub->GetInstallationTime();
    js_sub.last_installation_time =
        installation_time.is_null()
            ? ""
            : base::TimeFormatAsIso8601(sub->GetInstallationTime());
    result.emplace_back(std::move(js_sub));
  }
  return result;
}

content::BrowserContext* GetOriginalBrowserContext(
    content::BrowserContext* browser_context) {
  return Profile::FromBrowserContext(browser_context)->GetOriginalProfile();
}

}  // namespace

class EyeoFilteringPrivateAPI::EyeoFilteringAPIEventRouter
    : public adblock::ResourceClassificationRunner::Observer,
      public adblock::SubscriptionService::SubscriptionObserver,
      public adblock::FilteringConfiguration::Observer {
 public:
  explicit EyeoFilteringAPIEventRouter(content::BrowserContext* context)
      : context_(GetOriginalBrowserContext(context)) {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context_)
        ->AddObserver(this);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(context_);
    subscription_service->AddObserver(this);
    for (auto* it :
         subscription_service->GetInstalledFilteringConfigurations()) {
      it->AddObserver(this);
    }
  }

  ~EyeoFilteringAPIEventRouter() override {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context_)
        ->RemoveObserver(this);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(context_);
    subscription_service->RemoveObserver(this);
    for (auto* it :
         subscription_service->GetInstalledFilteringConfigurations()) {
      it->RemoveObserver(this);
    }
  }

  // adblock::ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        adblock::FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        adblock::ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {
    std::unique_ptr<Event> event;
    api::eyeo_filtering_private::RequestInfo info = CreateRequestInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = adblock::utils::ConvertURLs(parent_frame_urls);
    info.content_type = adblock::ContentTypeToString(content_type);

    if (match_result == adblock::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::EYEO_EVENT,
          api::eyeo_filtering_private::OnRequestBlocked::kEventName,
          api::eyeo_filtering_private::OnRequestBlocked::Create(info));
    } else {
      DCHECK(match_result == adblock::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::EYEO_EVENT,
          api::eyeo_filtering_private::OnRequestAllowed::kEventName,
          api::eyeo_filtering_private::OnRequestAllowed::Create(info));
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {
    api::eyeo_filtering_private::RequestInfo info = CreateRequestInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{};
    info.content_type = "";

    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnPageAllowed::kEventName,
        api::eyeo_filtering_private::OnPageAllowed::Create(info));

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPopupMatched(const GURL& url,
                      adblock::FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {
    std::unique_ptr<Event> event;
    api::eyeo_filtering_private::RequestInfo info = CreateRequestInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{opener_url.spec()};
    info.content_type = "";

    if (match_result == adblock::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::EYEO_EVENT,
          api::eyeo_filtering_private::OnPopupBlocked::kEventName,
          api::eyeo_filtering_private::OnPopupBlocked::Create(info));
    } else {
      DCHECK(match_result == adblock::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::EYEO_EVENT,
          api::eyeo_filtering_private::OnPopupAllowed::kEventName,
          api::eyeo_filtering_private::OnPopupAllowed::Create(info));
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPageElementMatched(
      const std::string& selector,
      adblock::ElementHideAction action,
      content::RenderFrameHost* render_frame_host) override {
    api::eyeo_filtering_private::PageElementInfo info =
        CreatePageElementInfoObject(selector, action, render_frame_host);

    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnPageElementMatched::kEventName,
        api::eyeo_filtering_private::OnPageElementMatched::Create(info));

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  // adblock::SubscriptionService::SubscriptionObserver:
  void OnSubscriptionInstalled(const GURL& url) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnSubscriptionUpdated::kEventName,
        api::eyeo_filtering_private::OnSubscriptionUpdated::Create(url.spec()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnFilteringConfigurationInstalled(
      adblock::FilteringConfiguration* config) override {
    config->AddObserver(this);
  }

  // adblock::FilteringConfiguration::Observer:
  void OnEnabledStateChanged(adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnEnabledStateChanged::kEventName,
        api::eyeo_filtering_private::OnEnabledStateChanged::Create(
            config->GetName()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnFilterListsChanged(adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnFilterListsChanged::kEventName,
        api::eyeo_filtering_private::OnFilterListsChanged::Create(
            config->GetName()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnAllowedDomainsChanged(
      adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnAllowedDomainsChanged::kEventName,
        api::eyeo_filtering_private::OnAllowedDomainsChanged::Create(
            config->GetName()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnCustomFiltersChanged(
      adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::eyeo_filtering_private::OnCustomFiltersChanged::kEventName,
        api::eyeo_filtering_private::OnCustomFiltersChanged::Create(
            config->GetName()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

 private:
  api::eyeo_filtering_private::RequestInfo CreateRequestInfoObject(
      const GURL& url,
      const GURL& subscription,
      const std::string& configuration_name,
      content::RenderFrameHost* render_frame_host) {
    DCHECK(render_frame_host);
    api::eyeo_filtering_private::RequestInfo info;
    info.url = url.spec();
    info.subscription = subscription.spec();
    info.configuration_name = configuration_name;
    info.tab_id = api::tabs::TAB_ID_NONE;
    info.window_id = SessionID::InvalidValue().id();
    const content::WebContents* wc =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    if (wc) {
      info.tab_id = ExtensionTabUtil::GetTabId(wc);
      info.window_id = ExtensionTabUtil::GetWindowIdOfTab(wc);
    }
    return info;
  }

  api::eyeo_filtering_private::PageElementInfo CreatePageElementInfoObject(
      const std::string& selector,
      adblock::ElementHideAction action,
      content::RenderFrameHost* render_frame_host) {
    DCHECK(render_frame_host);
    api::eyeo_filtering_private::PageElementInfo info;
    info.selector = selector;
    info.action =
        static_cast<api::eyeo_filtering_private::ElementHideAction>(action);
    info.tab_id = api::tabs::TAB_ID_NONE;
    info.window_id = SessionID::InvalidValue().id();
    const content::WebContents* wc =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    if (wc) {
      info.tab_id = ExtensionTabUtil::GetTabId(wc);
      info.window_id = ExtensionTabUtil::GetWindowIdOfTab(wc);
    }
    return info;
  }

  const raw_ptr<content::BrowserContext> context_;
};

template <>
void BrowserContextKeyedAPIFactory<
    EyeoFilteringPrivateAPI>::DeclareFactoryDependencies() {
  DependsOn(adblock::SubscriptionServiceFactory::GetInstance());
}

// static
BrowserContextKeyedAPIFactory<EyeoFilteringPrivateAPI>*
EyeoFilteringPrivateAPI::GetFactoryInstance() {
  static base::NoDestructor<
      BrowserContextKeyedAPIFactory<EyeoFilteringPrivateAPI>>
      instance;
  return instance.get();
}

// static
EyeoFilteringPrivateAPI* EyeoFilteringPrivateAPI::Get(
    content::BrowserContext* context) {
  return GetFactoryInstance()->Get(context);
}

EyeoFilteringPrivateAPI::EyeoFilteringPrivateAPI(
    content::BrowserContext* context)
    : context_(context) {
  // EventRouter can be null in tests
  auto* ev = EventRouter::Get(context_);
  if (ev) {
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnRequestAllowed::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnRequestBlocked::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnPageAllowed::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnPopupAllowed::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnPopupBlocked::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnSubscriptionUpdated::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnEnabledStateChanged::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnFilterListsChanged::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnAllowedDomainsChanged::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnCustomFiltersChanged::kEventName);
    ev->RegisterObserver(
        this, api::eyeo_filtering_private::OnPageElementMatched::kEventName);
  }
  // Make sure SessionStats is created so it will start collectings stats
  adblock::SessionStatsFactory::GetForBrowserContext(context);
}

EyeoFilteringPrivateAPI::~EyeoFilteringPrivateAPI() = default;

void EyeoFilteringPrivateAPI::Shutdown() {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->UnregisterObserver(this);
  }
  event_router_.reset();
}

void EyeoFilteringPrivateAPI::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  event_router_ =
      std::make_unique<EyeoFilteringPrivateAPI::EyeoFilteringAPIEventRouter>(
          context_);
  EventRouter::Get(context_)->UnregisterObserver(this);
}

namespace api {

EyeoFilteringPrivateCreateConfigurationFunction::
    EyeoFilteringPrivateCreateConfigurationFunction() = default;

EyeoFilteringPrivateCreateConfigurationFunction::
    ~EyeoFilteringPrivateCreateConfigurationFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateCreateConfigurationFunction::Run() {
  absl::optional<api::eyeo_filtering_private::CreateConfiguration::Params>
      params(api::eyeo_filtering_private::CreateConfiguration::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  const auto installed_configurations =
      subscription_service->GetInstalledFilteringConfigurations();
  auto configuration_it =
      std::ranges::find(installed_configurations, params->config_name,
                        &adblock::FilteringConfiguration::GetName);
  if (configuration_it == installed_configurations.end()) {
    auto new_filtering_configuration =
        std::make_unique<adblock::PersistentFilteringConfiguration>(
            Profile::FromBrowserContext(
                GetOriginalBrowserContext(browser_context()))
                ->GetPrefs(),
            params->config_name);
    subscription_service->InstallFilteringConfiguration(
        std::move(new_filtering_configuration));
  }
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateRemoveConfigurationFunction::
    EyeoFilteringPrivateRemoveConfigurationFunction() = default;

EyeoFilteringPrivateRemoveConfigurationFunction::
    ~EyeoFilteringPrivateRemoveConfigurationFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateRemoveConfigurationFunction::Run() {
  absl::optional<api::eyeo_filtering_private::RemoveConfiguration::Params>
      params(api::eyeo_filtering_private::RemoveConfiguration::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  const auto installed_configurations =
      subscription_service->GetInstalledFilteringConfigurations();
  auto configuration_it =
      std::ranges::find(installed_configurations, params->config_name,
                        &adblock::FilteringConfiguration::GetName);
  if (configuration_it != installed_configurations.end()) {
    subscription_service->UninstallFilteringConfiguration(
        std::string((*configuration_it)->GetName()));
  }
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateGetConfigurationsFunction::
    EyeoFilteringPrivateGetConfigurationsFunction() = default;

EyeoFilteringPrivateGetConfigurationsFunction::
    ~EyeoFilteringPrivateGetConfigurationsFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetConfigurationsFunction::Run() {
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  const auto installed_configurations =
      subscription_service->GetInstalledFilteringConfigurations();
  std::vector<std::string> configurations;
  std::ranges::transform(installed_configurations,
                         std::back_inserter(configurations),
                         [](adblock::FilteringConfiguration* config) {
                           return config->GetName();
                         });
  return RespondNow(ArgumentList(
      api::eyeo_filtering_private::GetConfigurations::Results::Create(
          std::move(configurations))));
}

EyeoFilteringPrivateSetAutoInstallEnabledFunction::
    EyeoFilteringPrivateSetAutoInstallEnabledFunction() {}

EyeoFilteringPrivateSetAutoInstallEnabledFunction::
    ~EyeoFilteringPrivateSetAutoInstallEnabledFunction() {}

ExtensionFunction::ResponseAction
EyeoFilteringPrivateSetAutoInstallEnabledFunction::Run() {
  absl::optional<api::eyeo_filtering_private::SetAutoInstallEnabled::Params>
      params =
          api::eyeo_filtering_private::SetAutoInstallEnabled::Params::Create(
              args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  subscription_service->SetAutoInstallEnabled(params->enabled);

  return RespondNow(NoArguments());
}

EyeoFilteringPrivateIsAutoInstallEnabledFunction::
    EyeoFilteringPrivateIsAutoInstallEnabledFunction() {}

EyeoFilteringPrivateIsAutoInstallEnabledFunction::
    ~EyeoFilteringPrivateIsAutoInstallEnabledFunction() {}

ExtensionFunction::ResponseAction
EyeoFilteringPrivateIsAutoInstallEnabledFunction::Run() {
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));

  return RespondNow(ArgumentList(
      api::eyeo_filtering_private::IsAutoInstallEnabled::Results::Create(
          subscription_service->IsAutoInstallEnabled())));
}

EyeoFilteringPrivateSetEnabledFunction::
    EyeoFilteringPrivateSetEnabledFunction() = default;

EyeoFilteringPrivateSetEnabledFunction::
    ~EyeoFilteringPrivateSetEnabledFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateSetEnabledFunction::Run() {
  absl::optional<api::eyeo_filtering_private::SetEnabled::Params> params(
      api::eyeo_filtering_private::SetEnabled::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  configuration->SetEnabled(params->enabled);
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateIsEnabledFunction::EyeoFilteringPrivateIsEnabledFunction() {
}

EyeoFilteringPrivateIsEnabledFunction::
    ~EyeoFilteringPrivateIsEnabledFunction() = default;

ExtensionFunction::ResponseAction EyeoFilteringPrivateIsEnabledFunction::Run() {
  absl::optional<api::eyeo_filtering_private::IsEnabled::Params> params(
      api::eyeo_filtering_private::IsEnabled::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  return RespondNow(
      ArgumentList(api::eyeo_filtering_private::IsEnabled::Results::Create(
          configuration->IsEnabled())));
}

EyeoFilteringPrivateGetAcceptableAdsUrlFunction::
    EyeoFilteringPrivateGetAcceptableAdsUrlFunction() = default;

EyeoFilteringPrivateGetAcceptableAdsUrlFunction::
    ~EyeoFilteringPrivateGetAcceptableAdsUrlFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetAcceptableAdsUrlFunction::Run() {
  return RespondNow(ArgumentList(
      api::eyeo_filtering_private::GetAcceptableAdsUrl::Results::Create(
          adblock::AcceptableAdsUrl().spec())));
}

EyeoFilteringPrivateAddFilterListFunction::
    EyeoFilteringPrivateAddFilterListFunction() = default;

EyeoFilteringPrivateAddFilterListFunction::
    ~EyeoFilteringPrivateAddFilterListFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateAddFilterListFunction::Run() {
  absl::optional<api::eyeo_filtering_private::AddFilterList::Params> params(
      api::eyeo_filtering_private::AddFilterList::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  auto url = GURL{params->url};
  auto status =
      RunSubscriptionAction(configuration, SubscriptionAction::kInstall, url);
  if (!status.empty()) {
    return RespondNow(Error(status));
  }

  return RespondNow(NoArguments());
}

EyeoFilteringPrivateRemoveFilterListFunction::
    EyeoFilteringPrivateRemoveFilterListFunction() = default;

EyeoFilteringPrivateRemoveFilterListFunction::
    ~EyeoFilteringPrivateRemoveFilterListFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateRemoveFilterListFunction::Run() {
  absl::optional<api::eyeo_filtering_private::RemoveFilterList::Params> params(
      api::eyeo_filtering_private::RemoveFilterList::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  auto url = GURL{params->url};
  auto status =
      RunSubscriptionAction(configuration, SubscriptionAction::kUninstall, url);
  if (!status.empty()) {
    return RespondNow(Error(status));
  }

  return RespondNow(NoArguments());
}

EyeoFilteringPrivateGetFilterListsFunction::
    EyeoFilteringPrivateGetFilterListsFunction() = default;

EyeoFilteringPrivateGetFilterListsFunction::
    ~EyeoFilteringPrivateGetFilterListsFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetFilterListsFunction::Run() {
  absl::optional<api::eyeo_filtering_private::GetFilterLists::Params> params(
      api::eyeo_filtering_private::GetFilterLists::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::eyeo_filtering_private::GetFilterLists::Results::Create(
          CopySubscriptions(
              subscription_service->GetCurrentSubscriptions(configuration)))));
}

EyeoFilteringPrivateAddAllowedDomainFunction::
    EyeoFilteringPrivateAddAllowedDomainFunction() = default;

EyeoFilteringPrivateAddAllowedDomainFunction::
    ~EyeoFilteringPrivateAddAllowedDomainFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateAddAllowedDomainFunction::Run() {
  absl::optional<api::eyeo_filtering_private::AddAllowedDomain::Params> params(
      api::eyeo_filtering_private::AddAllowedDomain::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  configuration->AddAllowedDomain(params->domain);
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateRemoveAllowedDomainFunction::
    EyeoFilteringPrivateRemoveAllowedDomainFunction() = default;

EyeoFilteringPrivateRemoveAllowedDomainFunction::
    ~EyeoFilteringPrivateRemoveAllowedDomainFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateRemoveAllowedDomainFunction::Run() {
  absl::optional<api::eyeo_filtering_private::RemoveAllowedDomain::Params>
      params(api::eyeo_filtering_private::RemoveAllowedDomain::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  configuration->RemoveAllowedDomain(params->domain);
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateGetAllowedDomainsFunction::
    EyeoFilteringPrivateGetAllowedDomainsFunction() = default;

EyeoFilteringPrivateGetAllowedDomainsFunction::
    ~EyeoFilteringPrivateGetAllowedDomainsFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetAllowedDomainsFunction::Run() {
  absl::optional<api::eyeo_filtering_private::GetAllowedDomains::Params> params(
      api::eyeo_filtering_private::GetAllowedDomains::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  return RespondNow(ArgumentList(
      api::eyeo_filtering_private::GetCustomFilters::Results::Create(
          configuration->GetAllowedDomains())));
}

EyeoFilteringPrivateAddCustomFilterFunction::
    EyeoFilteringPrivateAddCustomFilterFunction() = default;

EyeoFilteringPrivateAddCustomFilterFunction::
    ~EyeoFilteringPrivateAddCustomFilterFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateAddCustomFilterFunction::Run() {
  absl::optional<api::eyeo_filtering_private::AddCustomFilter::Params> params(
      api::eyeo_filtering_private::AddCustomFilter::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  configuration->AddCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

EyeoFilteringPrivateRemoveCustomFilterFunction::
    EyeoFilteringPrivateRemoveCustomFilterFunction() = default;

EyeoFilteringPrivateRemoveCustomFilterFunction::
    ~EyeoFilteringPrivateRemoveCustomFilterFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateRemoveCustomFilterFunction::Run() {
  absl::optional<api::eyeo_filtering_private::RemoveCustomFilter::Params>
      params(api::eyeo_filtering_private::RemoveCustomFilter::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  configuration->RemoveCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetCustomFiltersFunction::Run() {
  absl::optional<api::eyeo_filtering_private::GetCustomFilters::Params> params(
      api::eyeo_filtering_private::GetCustomFilters::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* configuration = FindFilteringConfiguration(
      GetOriginalBrowserContext(browser_context()), params->configuration);
  if (!configuration) {
    return RespondNow(Error(base::StringPrintf(kConfigurationMissing,
                                               params->configuration.c_str())));
  }
  return RespondNow(ArgumentList(
      api::eyeo_filtering_private::GetCustomFilters::Results::Create(
          configuration->GetCustomFilters())));
}

EyeoFilteringPrivateGetCustomFiltersFunction::
    EyeoFilteringPrivateGetCustomFiltersFunction() = default;

EyeoFilteringPrivateGetCustomFiltersFunction::
    ~EyeoFilteringPrivateGetCustomFiltersFunction() = default;

EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction::
    EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction() = default;

EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction::
    ~EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction::Run() {
  auto* session_stats = adblock::SessionStatsFactory::GetForBrowserContext(
      GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::eyeo_filtering_private::GetSessionAllowedRequestsCount::
                       Results::Create(CopySessionsStats(
                           session_stats->GetSessionAllowedResourcesCount()))));
}

EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction::
    EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction() = default;

EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction::
    ~EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction() = default;

ExtensionFunction::ResponseAction
EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction::Run() {
  auto* session_stats = adblock::SessionStatsFactory::GetForBrowserContext(
      GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::eyeo_filtering_private::GetSessionBlockedRequestsCount::
                       Results::Create(CopySessionsStats(
                           session_stats->GetSessionBlockedResourcesCount()))));
}
}  // namespace api
}  // namespace extensions
