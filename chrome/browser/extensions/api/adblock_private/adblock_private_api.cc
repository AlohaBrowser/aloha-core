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

#include "chrome/browser/extensions/api/adblock_private/adblock_private_api.h"

#include "base/containers/flat_map.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/tabs.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/session_stats_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/session_stats.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace extensions {

namespace {

enum class SubscriptionAction { kInstall, kUninstall };

content::BrowserContext* GetOriginalBrowserContext(
    content::BrowserContext* browser_context) {
  return Profile::FromBrowserContext(browser_context)->GetOriginalProfile();
}

adblock::FilteringConfiguration* GetAdblockConfiguration(
    content::BrowserContext* browser_context) {
  auto* adblock_configuration =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context))
          ->GetFilteringConfiguration(
              adblock::kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration)
      << "adblock_private expects \"adblock\" configuration";
  return adblock_configuration;
}

std::string RunSubscriptionAction(SubscriptionAction action,
                                  content::BrowserContext* browser_context,
                                  const GURL& url) {
  if (!url.is_valid()) {
    return "Invalid URL";
  }
  auto* adblock_configuration = GetAdblockConfiguration(browser_context);
  switch (action) {
    case SubscriptionAction::kInstall:
      adblock_configuration->AddFilterList(url);
      break;
    case SubscriptionAction::kUninstall:
      adblock_configuration->RemoveFilterList(url);
      break;
    default:
      NOTREACHED();
  }

  return {};
}

std::vector<api::adblock_private::SessionStatsEntry> CopySessionsStats(
    const std::map<GURL, long>& source) {
  std::vector<api::adblock_private::SessionStatsEntry> result;
  for (auto& entry : source) {
    api::adblock_private::SessionStatsEntry js_entry;
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

std::vector<api::adblock_private::Subscription> CopySubscriptions(
    const std::vector<scoped_refptr<adblock::Subscription>>
        current_subscriptions) {
  std::vector<api::adblock_private::Subscription> result;
  for (auto& sub : current_subscriptions) {
    api::adblock_private::Subscription js_sub;
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

}  // namespace

template <>
void BrowserContextKeyedAPIFactory<
    AdblockPrivateAPI>::DeclareFactoryDependencies() {
  DependsOn(adblock::SubscriptionServiceFactory::GetInstance());
  DependsOn(adblock::ResourceClassificationRunnerFactory::GetInstance());
  DependsOn(adblock::SessionStatsFactory::GetInstance());
}

// static
BrowserContextKeyedAPIFactory<AdblockPrivateAPI>*
AdblockPrivateAPI::GetFactoryInstance() {
  static base::NoDestructor<BrowserContextKeyedAPIFactory<AdblockPrivateAPI>>
      instance;
  return instance.get();
}

class AdblockPrivateAPI::AdblockAPIEventRouter
    : public adblock::ResourceClassificationRunner::Observer,
      public adblock::SubscriptionService::SubscriptionObserver,
      public adblock::FilteringConfiguration::Observer {
 public:
  explicit AdblockAPIEventRouter(content::BrowserContext* context)
      : context_(GetOriginalBrowserContext(context)) {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context_)
        ->AddObserver(this);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(context_);
    subscription_service->AddObserver(this);
    GetAdblockConfiguration(context_)->AddObserver(this);
  }

  ~AdblockAPIEventRouter() override {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context_)
        ->RemoveObserver(this);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(context_);
    subscription_service->RemoveObserver(this);
    GetAdblockConfiguration(context_)->RemoveObserver(this);
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
    api::adblock_private::AdInfo info = CreateAdInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = adblock::utils::ConvertURLs(parent_frame_urls);
    info.content_type = adblock::ContentTypeToString(content_type);

    if (match_result == adblock::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::EYEO_EVENT, api::adblock_private::OnAdBlocked::kEventName,
          api::adblock_private::OnAdBlocked::Create(info));
    } else {
      DCHECK(match_result == adblock::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::EYEO_EVENT, api::adblock_private::OnAdAllowed::kEventName,
          api::adblock_private::OnAdAllowed::Create(info));
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {
    api::adblock_private::AdInfo info = CreateAdInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{};
    info.content_type = "";

    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT, api::adblock_private::OnPageAllowed::kEventName,
        api::adblock_private::OnPageAllowed::Create(info));

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPopupMatched(const GURL& url,
                      adblock::FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {
    std::unique_ptr<Event> event;
    api::adblock_private::AdInfo info = CreateAdInfoObject(
        url, subscription, configuration_name, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{opener_url.spec()};
    info.content_type = "";

    if (match_result == adblock::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::EYEO_EVENT, api::adblock_private::OnPopupBlocked::kEventName,
          api::adblock_private::OnPopupBlocked::Create(info));
    } else {
      DCHECK(match_result == adblock::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::EYEO_EVENT, api::adblock_private::OnPopupAllowed::kEventName,
          api::adblock_private::OnPopupAllowed::Create(info));
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  // adblock::SubscriptionService::SubscriptionObserver:
  void OnSubscriptionInstalled(const GURL& url) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::adblock_private::OnSubscriptionUpdated::kEventName,
        api::adblock_private::OnSubscriptionUpdated::Create(url.spec()));
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  // adblock::FilteringConfiguration::Observer:
  void OnEnabledStateChanged(adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::adblock_private::OnEnabledStateChanged::kEventName,
        api::adblock_private::OnEnabledStateChanged::Create());
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnFilterListsChanged(adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::adblock_private::OnFilterListsChanged::kEventName,
        api::adblock_private::OnFilterListsChanged::Create());
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnAllowedDomainsChanged(
      adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::adblock_private::OnAllowedDomainsChanged::kEventName,
        api::adblock_private::OnAllowedDomainsChanged::Create());
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnCustomFiltersChanged(
      adblock::FilteringConfiguration* config) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::EYEO_EVENT,
        api::adblock_private::OnCustomFiltersChanged::kEventName,
        api::adblock_private::OnCustomFiltersChanged::Create());
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

 private:
  api::adblock_private::AdInfo CreateAdInfoObject(
      const GURL& url,
      const GURL& subscription,
      const std::string& configuration_name,
      content::RenderFrameHost* render_frame_host) {
    DCHECK(render_frame_host);
    api::adblock_private::AdInfo info;
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

  raw_ptr<content::BrowserContext> context_;
};

void AdblockPrivateAPI::Shutdown() {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->UnregisterObserver(this);
  }
  event_router_.reset();
}

// static
AdblockPrivateAPI* AdblockPrivateAPI::Get(content::BrowserContext* context) {
  return GetFactoryInstance()->Get(context);
}

AdblockPrivateAPI::AdblockPrivateAPI(content::BrowserContext* context)
    : context_(context) {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnAdAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnAdBlocked::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPageAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPopupAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPopupBlocked::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnSubscriptionUpdated::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnEnabledStateChanged::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnFilterListsChanged::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnAllowedDomainsChanged::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnCustomFiltersChanged::kEventName);
  }
  // Make sure SessionStats is created so it will start collectings stats
  adblock::SessionStatsFactory::GetForBrowserContext(context);
}

AdblockPrivateAPI::~AdblockPrivateAPI() = default;

void AdblockPrivateAPI::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  event_router_ =
      std::make_unique<AdblockPrivateAPI::AdblockAPIEventRouter>(context_);
  EventRouter::Get(context_)->UnregisterObserver(this);
}

namespace api {

AdblockPrivateSetEnabledFunction::AdblockPrivateSetEnabledFunction() {}

AdblockPrivateSetEnabledFunction::~AdblockPrivateSetEnabledFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateSetEnabledFunction::Run() {
  absl::optional<api::adblock_private::SetEnabled::Params> params =
      api::adblock_private::SetEnabled::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  adblock_configuration->SetEnabled(params->enabled);
  return RespondNow(NoArguments());
}

AdblockPrivateIsEnabledFunction::AdblockPrivateIsEnabledFunction() {}

AdblockPrivateIsEnabledFunction::~AdblockPrivateIsEnabledFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateIsEnabledFunction::Run() {
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::adblock_private::IsEnabled::Results::Create(
          adblock_configuration->IsEnabled())));
}

AdblockPrivateSetAcceptableAdsEnabledFunction::
    AdblockPrivateSetAcceptableAdsEnabledFunction() {}

AdblockPrivateSetAcceptableAdsEnabledFunction::
    ~AdblockPrivateSetAcceptableAdsEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateSetAcceptableAdsEnabledFunction::Run() {
  absl::optional<api::adblock_private::SetAcceptableAdsEnabled::Params> params =
      api::adblock_private::SetAcceptableAdsEnabled::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  if (params->enabled) {
    adblock_configuration->AddFilterList(adblock::AcceptableAdsUrl());
  } else {
    adblock_configuration->RemoveFilterList(adblock::AcceptableAdsUrl());
  }

  return RespondNow(NoArguments());
}

AdblockPrivateIsAcceptableAdsEnabledFunction::
    AdblockPrivateIsAcceptableAdsEnabledFunction() {}

AdblockPrivateIsAcceptableAdsEnabledFunction::
    ~AdblockPrivateIsAcceptableAdsEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateIsAcceptableAdsEnabledFunction::Run() {
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  return RespondNow(ArgumentList(
      api::adblock_private::IsAcceptableAdsEnabled::Results::Create(
          std::ranges::any_of(adblock_configuration->GetFilterLists(),
                              [&](const auto& url) {
                                return url == adblock::AcceptableAdsUrl();
                              }))));
}

AdblockPrivateSetAutoInstallEnabledFunction::
    AdblockPrivateSetAutoInstallEnabledFunction() {}

AdblockPrivateSetAutoInstallEnabledFunction::
    ~AdblockPrivateSetAutoInstallEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateSetAutoInstallEnabledFunction::Run() {
  absl::optional<api::adblock_private::SetAutoInstallEnabled::Params> params =
      api::adblock_private::SetAutoInstallEnabled::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));

  subscription_service->SetAutoInstallEnabled(params->enabled);

  return RespondNow(NoArguments());
}

AdblockPrivateIsAutoInstallEnabledFunction::
    AdblockPrivateIsAutoInstallEnabledFunction() {}

AdblockPrivateIsAutoInstallEnabledFunction::
    ~AdblockPrivateIsAutoInstallEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateIsAutoInstallEnabledFunction::Run() {
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));

  return RespondNow(
      ArgumentList(api::adblock_private::IsAutoInstallEnabled::Results::Create(
          subscription_service->IsAutoInstallEnabled())));
}

AdblockPrivateGetBuiltInSubscriptionsFunction::
    AdblockPrivateGetBuiltInSubscriptionsFunction() {}

AdblockPrivateGetBuiltInSubscriptionsFunction::
    ~AdblockPrivateGetBuiltInSubscriptionsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetBuiltInSubscriptionsFunction::Run() {
  auto recommended = adblock::config::GetKnownSubscriptions();
  std::vector<api::adblock_private::BuiltInSubscription> result;
  for (auto& recommended_one : recommended) {
    if (recommended_one.ui_visibility ==
        adblock::SubscriptionUiVisibility::Visible) {
      api::adblock_private::BuiltInSubscription js_recommended;
      js_recommended.url = recommended_one.url.spec();
      js_recommended.title = recommended_one.title;
      js_recommended.languages = recommended_one.languages;
      result.emplace_back(std::move(js_recommended));
    }
  }
  return RespondNow(ArgumentList(
      api::adblock_private::GetBuiltInSubscriptions::Results::Create(result)));
}

AdblockPrivateInstallSubscriptionFunction::
    AdblockPrivateInstallSubscriptionFunction() {}

AdblockPrivateInstallSubscriptionFunction::
    ~AdblockPrivateInstallSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateInstallSubscriptionFunction::Run() {
  absl::optional<api::adblock_private::InstallSubscription::Params> params =
      api::adblock_private::InstallSubscription::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status =
      RunSubscriptionAction(SubscriptionAction::kInstall,
                            GetOriginalBrowserContext(browser_context()), url);
  if (!status.empty()) {
    return RespondNow(Error(status));
  }

  return RespondNow(NoArguments());
}

AdblockPrivateUninstallSubscriptionFunction::
    AdblockPrivateUninstallSubscriptionFunction() {}

AdblockPrivateUninstallSubscriptionFunction::
    ~AdblockPrivateUninstallSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateUninstallSubscriptionFunction::Run() {
  absl::optional<api::adblock_private::UninstallSubscription::Params> params =
      api::adblock_private::UninstallSubscription::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status =
      RunSubscriptionAction(SubscriptionAction::kUninstall,
                            GetOriginalBrowserContext(browser_context()), url);
  if (!status.empty()) {
    return RespondNow(Error(status));
  }

  return RespondNow(NoArguments());
}

AdblockPrivateGetInstalledSubscriptionsFunction::
    AdblockPrivateGetInstalledSubscriptionsFunction() {}

AdblockPrivateGetInstalledSubscriptionsFunction::
    ~AdblockPrivateGetInstalledSubscriptionsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetInstalledSubscriptionsFunction::Run() {
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          GetOriginalBrowserContext(browser_context()));
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  return RespondNow(ArgumentList(
      api::adblock_private::GetInstalledSubscriptions::Results::Create(
          CopySubscriptions(subscription_service->GetCurrentSubscriptions(
              adblock_configuration)))));
}

AdblockPrivateAddAllowedDomainFunction::
    AdblockPrivateAddAllowedDomainFunction() {}

AdblockPrivateAddAllowedDomainFunction::
    ~AdblockPrivateAddAllowedDomainFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateAddAllowedDomainFunction::Run() {
  absl::optional<api::adblock_private::AddAllowedDomain::Params> params =
      api::adblock_private::AddAllowedDomain::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  adblock_configuration->AddAllowedDomain(params->domain);
  return RespondNow(NoArguments());
}

AdblockPrivateRemoveAllowedDomainFunction::
    AdblockPrivateRemoveAllowedDomainFunction() {}

AdblockPrivateRemoveAllowedDomainFunction::
    ~AdblockPrivateRemoveAllowedDomainFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateRemoveAllowedDomainFunction::Run() {
  absl::optional<api::adblock_private::RemoveAllowedDomain::Params> params =
      api::adblock_private::RemoveAllowedDomain::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  adblock_configuration->RemoveAllowedDomain(params->domain);

  return RespondNow(NoArguments());
}

AdblockPrivateGetAllowedDomainsFunction::
    AdblockPrivateGetAllowedDomainsFunction() {}

AdblockPrivateGetAllowedDomainsFunction::
    ~AdblockPrivateGetAllowedDomainsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetAllowedDomainsFunction::Run() {
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::adblock_private::GetAllowedDomains::Results::Create(
          adblock_configuration->GetAllowedDomains())));
}

AdblockPrivateAddCustomFilterFunction::AdblockPrivateAddCustomFilterFunction() {
}

AdblockPrivateAddCustomFilterFunction::
    ~AdblockPrivateAddCustomFilterFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateAddCustomFilterFunction::Run() {
  absl::optional<api::adblock_private::AddCustomFilter::Params> params =
      api::adblock_private::AddCustomFilter::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  adblock_configuration->AddCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

AdblockPrivateRemoveCustomFilterFunction::
    AdblockPrivateRemoveCustomFilterFunction() {}

AdblockPrivateRemoveCustomFilterFunction::
    ~AdblockPrivateRemoveCustomFilterFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetCustomFiltersFunction::Run() {
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  return RespondNow(
      ArgumentList(api::adblock_private::GetCustomFilters::Results::Create(
          adblock_configuration->GetCustomFilters())));
}

AdblockPrivateGetCustomFiltersFunction::
    AdblockPrivateGetCustomFiltersFunction() {}

AdblockPrivateGetCustomFiltersFunction::
    ~AdblockPrivateGetCustomFiltersFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateRemoveCustomFilterFunction::Run() {
  absl::optional<api::adblock_private::RemoveCustomFilter::Params> params =
      api::adblock_private::RemoveCustomFilter::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* adblock_configuration =
      GetAdblockConfiguration(GetOriginalBrowserContext(browser_context()));
  adblock_configuration->RemoveCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

AdblockPrivateGetSessionAllowedAdsCountFunction::
    AdblockPrivateGetSessionAllowedAdsCountFunction() {}

AdblockPrivateGetSessionAllowedAdsCountFunction::
    ~AdblockPrivateGetSessionAllowedAdsCountFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetSessionAllowedAdsCountFunction::Run() {
  auto* session_stats_ = adblock::SessionStatsFactory::GetForBrowserContext(
      GetOriginalBrowserContext(browser_context()));
  return RespondNow(ArgumentList(
      api::adblock_private::GetSessionAllowedAdsCount::Results::Create(
          CopySessionsStats(
              session_stats_->GetSessionAllowedResourcesCount()))));
}

AdblockPrivateGetSessionBlockedAdsCountFunction::
    AdblockPrivateGetSessionBlockedAdsCountFunction() {}

AdblockPrivateGetSessionBlockedAdsCountFunction::
    ~AdblockPrivateGetSessionBlockedAdsCountFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetSessionBlockedAdsCountFunction::Run() {
  auto* session_stats_ = adblock::SessionStatsFactory::GetForBrowserContext(
      GetOriginalBrowserContext(browser_context()));
  return RespondNow(ArgumentList(
      api::adblock_private::GetSessionAllowedAdsCount::Results::Create(
          CopySessionsStats(
              session_stats_->GetSessionBlockedResourcesCount()))));
}

}  // namespace api
}  // namespace extensions
