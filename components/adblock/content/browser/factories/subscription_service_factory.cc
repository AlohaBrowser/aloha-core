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

#include "components/adblock/content/browser/factories/subscription_service_factory.h"

#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

#include "absl/types/optional.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/content/browser/factories/subscription_persistent_metadata_factory.h"
#include "components/adblock/core/adblock_switches.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "components/adblock/core/subscription/filtering_configuration_maintainer_impl.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/ongoing_subscription_request_impl.h"
#include "components/adblock/core/subscription/preloaded_subscription_provider_impl.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_downloader_impl.h"
#include "components/adblock/core/subscription/subscription_persistent_storage_impl.h"
#include "components/adblock/core/subscription/subscription_service_impl.h"
#include "components/adblock/core/subscription/subscription_updater_impl.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/language/core/browser/locale_util.h"
#include "components/language/core/common/locale_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace adblock {
namespace {

std::optional<base::TimeDelta> g_update_check_interval_for_testing;
std::optional<base::TimeDelta> g_update_initial_delay_for_testing;

base::TimeDelta GetUpdateInitialDelay() {
  static base::TimeDelta kInitialDelay =
      g_update_initial_delay_for_testing
          ? g_update_initial_delay_for_testing.value()
          : base::Seconds(30);
  return kInitialDelay;
}

base::TimeDelta GetUpdateCheckInterval() {
  static base::TimeDelta kCheckInterval =
      g_update_check_interval_for_testing
          ? g_update_check_interval_for_testing.value()
          : base::Hours(1);
  return kCheckInterval;
}

constexpr net::BackoffEntry::Policy kRetryBackoffPolicy = {
    0,               // Number of initial errors to ignore.
    5000,            // Initial delay in ms.
    2.0,             // Factor by which the waiting time will be multiplied.
    0.2,             // Fuzzing percentage.
    60 * 60 * 1000,  // Maximum delay in ms.
    -1,              // Never discard the entry.
    false,           // Use initial delay.
};

std::unique_ptr<OngoingSubscriptionRequest> MakeOngoingSubscriptionRequest(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  return std::make_unique<OngoingSubscriptionRequestImpl>(&kRetryBackoffPolicy,
                                                          url_loader_factory);
}

ConversionResult ConvertFilterFile(const GURL& subscription_url,
                                   const base::FilePath& path) {
  TRACE_EVENT1("eyeo", "ConvertFileToFlatbuffer", "url",
               subscription_url.spec());
  ConversionResult result;
  std::ifstream input_stream(path.AsUTF8Unsafe());
  if (!input_stream.is_open() || !input_stream.good()) {
    result = ConversionError("Could not open filter file");
  } else {
    result = FlatbufferConverter::Convert(
        input_stream, subscription_url,
        config::AllowPrivilegedFilters(subscription_url));
  }
  base::DeleteFile(path);
  return result;
}

std::unique_ptr<SubscriptionUpdater> MakeSubscriptionUpdater() {
  return std::make_unique<SubscriptionUpdaterImpl>(GetUpdateInitialDelay(),
                                                   GetUpdateCheckInterval());
}

std::unique_ptr<FilteringConfigurationMaintainer>
MakeFilterConfigurationMaintainer(
    content::BrowserContext* context,
    PrefService* prefs,
    SubscriptionPersistentMetadata* persistent_metadata,
    const ConversionExecutors* conversion_executors,
    FilteringConfiguration* configuration,
    FilteringConfigurationMaintainerImpl::SubscriptionUpdatedCallback
        observer) {
  auto main_thread_task_runner = base::SequencedTaskRunner::GetCurrentDefault();
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess();

  const std::string storage_dir = configuration->GetName() + "_subscriptions";

  auto storage = std::make_unique<SubscriptionPersistentStorageImpl>(
      context->GetPath().AppendASCII(storage_dir),
      std::make_unique<SubscriptionValidatorImpl>(prefs,
                                                  CurrentSchemaVersion()),
      persistent_metadata);

  auto downloader = std::make_unique<SubscriptionDownloaderImpl>(
      utils::GetAppInfo(),
      base::BindRepeating(&MakeOngoingSubscriptionRequest, url_loader_factory),
      const_cast<ConversionExecutors*>(conversion_executors),
      persistent_metadata);

  auto maintainer = std::make_unique<FilteringConfigurationMaintainerImpl>(
      configuration, std::move(storage), std::move(downloader),
      std::make_unique<PreloadedSubscriptionProviderImpl>(),
      MakeSubscriptionUpdater(),
      const_cast<ConversionExecutors*>(conversion_executors),
      persistent_metadata, observer);
  maintainer->InitializeStorage();
  return maintainer;
}

void CleanupPersistedConfiguration(PrefService* prefs,
                                   FilteringConfiguration* configuration) {
  PersistentFilteringConfiguration::RemovePersistedData(
      prefs, configuration->GetName());
}

void InstallFirstRunDefaultAdblockSubscription(
    std::unique_ptr<FilteringConfiguration>& adblock_filtering_configuration) {
  if (base::ranges::any_of(adblock::config::GetKnownSubscriptions(),
                           [&](const KnownSubscriptionInfo& subscription) {
                             return subscription.url ==
                                        DefaultSubscriptionUrl() &&
                                    subscription.first_run !=
                                        SubscriptionFirstRunBehavior::Ignore;
                           })) {
    VLOG(1) << "[eyeo] Using the default subscription";
    adblock_filtering_configuration->AddFilterList(DefaultSubscriptionUrl());
  } else {
    VLOG(1) << "[eyeo] No default subscription found, neither "
               "language-specific, nor generic.";
  }
}

bool InstallFirstRunAdblockSubscriptionsCheckingLocale(
    std::string_view language,
    std::unique_ptr<FilteringConfiguration>& adblock_filtering_configuration) {
  bool language_specific_subscription_installed = false;
  // On first run, install additional subscriptions.
  for (const auto& subscription : adblock::config::GetKnownSubscriptions()) {
    if (subscription.first_run == SubscriptionFirstRunBehavior::Subscribe) {
      adblock_filtering_configuration->AddFilterList(subscription.url);
    } else if (subscription.first_run ==
                   SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch &&
               std::find(subscription.languages.begin(),
                         subscription.languages.end(),
                         language) != subscription.languages.end()) {
      VLOG(1) << "[eyeo] Using recommended subscription for language \""
              << language << "\": " << subscription.title;
      language_specific_subscription_installed = true;
      adblock_filtering_configuration->AddFilterList(subscription.url);
    }
  }
  return language_specific_subscription_installed;
}

void CheckAndRunFirstRunLogic(
    std::string_view locale,
    PrefService* prefs,
    std::unique_ptr<FilteringConfiguration>& adblock_filtering_configuration) {
  // If the state of installed subscriptions in SubscriptionService is different
  // than the state in prefs, prefs take precedence.
  if (prefs->GetBoolean(common::prefs::kInstallFirstStartSubscriptions)) {
    adblock_filtering_configuration =
        std::make_unique<PersistentFilteringConfiguration>(
            prefs, kAdblockFilteringConfigurationName);
    std::string_view language = language::ExtractBaseLanguage(locale);
    if (language.size() != 2u) {
      language = "en";
    }
    if (!InstallFirstRunAdblockSubscriptionsCheckingLocale(
            language, adblock_filtering_configuration)) {
      InstallFirstRunDefaultAdblockSubscription(
          adblock_filtering_configuration);
    }
    if (IsEyeoFilteringDisabledByDefault()) {
      adblock_filtering_configuration->SetEnabled(false);
    }
    if (IsAcceptableAdsDisabledByDefault()) {
      adblock_filtering_configuration->RemoveFilterList(AcceptableAdsUrl());
    }
    prefs->SetBoolean(common::prefs::kInstallFirstStartSubscriptions, false);
  }
}

template <typename T>
std::vector<T> MigrateItemsFromList(PrefService* pref_service,
                                    const std::string& pref_name) {
  std::vector<T> results;
  if (pref_service->FindPreference(pref_name)->HasUserSetting()) {
    const auto& list = pref_service->GetList(pref_name);
    for (const auto& item : list) {
      if (item.is_string()) {
        results.emplace_back(item.GetString());
      }
    }
    pref_service->ClearPref(pref_name);
  }
  return results;
}

absl::optional<bool> MigrateBoolFromPrefs(PrefService* pref_service,
                                          const std::string& pref_name) {
  if (pref_service->FindPreference(pref_name)->HasUserSetting()) {
    bool value = pref_service->GetBoolean(pref_name);
    pref_service->ClearPref(pref_name);
    return value;
  }
  return absl::nullopt;
}

void CheckAndMigrateSettings(
    PrefService* prefs,
    std::unique_ptr<FilteringConfiguration>& adblock_filtering_configuration) {
  auto enable_value =
      MigrateBoolFromPrefs(prefs, common::prefs::kEnableAdblockLegacy);
  auto aa_value =
      MigrateBoolFromPrefs(prefs, common::prefs::kEnableAcceptableAdsLegacy);
  auto subscriptions = MigrateItemsFromList<GURL>(
      prefs, common::prefs::kAdblockSubscriptionsLegacy);
  auto custom_subscriptions = MigrateItemsFromList<GURL>(
      prefs, common::prefs::kAdblockCustomSubscriptionsLegacy);
  auto domains = MigrateItemsFromList<std::string>(
      prefs, common::prefs::kAdblockAllowedDomainsLegacy);
  auto filters = MigrateItemsFromList<std::string>(
      prefs, common::prefs::kAdblockCustomFiltersLegacy);

  if (!enable_value && !aa_value && subscriptions.empty() &&
      custom_subscriptions.empty() && domains.empty() && filters.empty()) {
    return;
  }

  if (!adblock_filtering_configuration) {
    adblock_filtering_configuration =
        std::make_unique<PersistentFilteringConfiguration>(
            prefs, kAdblockFilteringConfigurationName);
  }

  if (enable_value) {
    adblock_filtering_configuration->SetEnabled(*enable_value);
    VLOG(1) << "[eyeo] Migrated kEnableAdblockLegacy pref";
  }

  if (aa_value) {
    if (*aa_value) {
      adblock_filtering_configuration->AddFilterList(AcceptableAdsUrl());
    } else {
      adblock_filtering_configuration->RemoveFilterList(AcceptableAdsUrl());
    }
    VLOG(1) << "[eyeo] Migrated kEnableAcceptableAdsLegacy pref";
  } else if (!subscriptions.empty()) {
    // In old version AA setting could never be touched but AA was in use.
    // So if we see no value for AA but kAdblockSubscriptionsLegacy pref was
    // touched (kAdblockSubscriptionsLegacy is touched during 1st run scenario)
    // we need to assume AA was enabled. See DPD-2219.
    adblock_filtering_configuration->AddFilterList(AcceptableAdsUrl());
  }

  for (const auto& url : subscriptions) {
    adblock_filtering_configuration->AddFilterList(url);
    VLOG(1) << "[eyeo] Migrated " << url
            << " from kAdblockSubscriptionsLegacy pref";
  }

  for (const auto& url : custom_subscriptions) {
    adblock_filtering_configuration->AddFilterList(url);
    VLOG(1) << "[eyeo] Migrated " << url
            << " from kAdblockCustomSubscriptionsLegacy pref";
  }

  for (const auto& domain : domains) {
    adblock_filtering_configuration->AddAllowedDomain(domain);
    VLOG(1) << "[eyeo] Migrated " << domain
            << " from kAdblockAllowedDomainsLegacy pref";
  }

  for (const auto& filter : filters) {
    adblock_filtering_configuration->AddCustomFilter(filter);
    VLOG(1) << "[eyeo] Migrated " << filter
            << " from kAdblockCustomFiltersLegacy pref";
  }
}

void CheckAdblockCliSwitches(
    FilteringConfiguration* adblock_filtering_configuration) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          adblock::switches::kDisableAcceptableAds)) {
    adblock_filtering_configuration->RemoveFilterList(AcceptableAdsUrl());
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableAdblock)) {
    adblock_filtering_configuration->SetEnabled(false);
  }
}

}  // namespace

SubscriptionServiceFactory::SubscriptionServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSubscriptionService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SubscriptionPersistentMetadataFactory::GetInstance());
}

SubscriptionServiceFactory::~SubscriptionServiceFactory() = default;

// static
SubscriptionService* SubscriptionServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SubscriptionService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
SubscriptionServiceFactory* SubscriptionServiceFactory::GetInstance() {
  static base::NoDestructor<SubscriptionServiceFactory> instance;
  return instance.get();
}

KeyedService* SubscriptionServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* prefs = GetPrefs(context);
  auto* subscription_service = new SubscriptionServiceImpl(
      base::BindRepeating(&MakeFilterConfigurationMaintainer, context, prefs,
                          GetSubscriptionPersistentMetadata(context),
                          static_cast<const ConversionExecutors*>(this)),
      base::BindRepeating(&CleanupPersistedConfiguration, prefs));
  auto persisted_configs =
      PersistentFilteringConfiguration::GetPersistedConfigurations(prefs);
  bool start_disabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableEyeoFiltering);
  for (auto& persisted_configuration : persisted_configs) {
    if (start_disabled) {
      persisted_configuration->SetEnabled(false);
    }
    if (persisted_configuration->GetName() ==
        kAdblockFilteringConfigurationName) {
      CheckAdblockCliSwitches(persisted_configuration.get());
    }
    subscription_service->InstallFilteringConfiguration(
        std::move(persisted_configuration));
  }
  if (persisted_configs.empty()) {
    std::unique_ptr<FilteringConfiguration> adblock_filtering_configuration;
    // Check if we have a 1st run scenario, if yes then "adblock"
    // FilterConfiguration will be created and configured.
    {
      base::ScopedAllowBlocking allow_blocking;
      CheckAndRunFirstRunLogic(GetLocale(prefs), prefs,
                               adblock_filtering_configuration);
    }
    // Check and migrate if there are any pre-configuration settings.
    // "adblock" FilterConfiguration will be created if needed.
    CheckAndMigrateSettings(prefs, adblock_filtering_configuration);
    if (adblock_filtering_configuration) {
      CheckAdblockCliSwitches(adblock_filtering_configuration.get());
      if (start_disabled) {
        adblock_filtering_configuration->SetEnabled(false);
      }
      subscription_service->InstallFilteringConfiguration(
          std::move(adblock_filtering_configuration));
    }
  }
  return subscription_service;
}

scoped_refptr<InstalledSubscription>
SubscriptionServiceFactory::ConvertCustomFilters(
    const std::vector<std::string>& filters) const {
  auto raw_data =
      FlatbufferConverter::Convert(filters, CustomFiltersUrl(), true);
  return base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(raw_data), Subscription::InstallationState::Installed,
      base::Time());
}

void SubscriptionServiceFactory::ConvertFilterListFile(
    const GURL& subscription_url,
    const base::FilePath& path,
    base::OnceCallback<void(ConversionResult)> result_callback) const {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ConvertFilterFile, subscription_url, path),
      std::move(result_callback));
}

void SubscriptionServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  adblock::common::prefs::RegisterProfilePrefs(registry);
}

// static
void SubscriptionServiceFactory::SetUpdateCheckAndDelayIntervalsForTesting(
    base::TimeDelta check_interval,
    base::TimeDelta initial_delay) {
  g_update_check_interval_for_testing = check_interval;
  g_update_initial_delay_for_testing = initial_delay;
}

SubscriptionPersistentMetadata*
SubscriptionServiceFactory::GetSubscriptionPersistentMetadata(
    content::BrowserContext* context) const {
  return SubscriptionPersistentMetadataFactory::GetForBrowserContext(context);
}

content::BrowserContext* SubscriptionServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

PrefService* SubscriptionServiceFactory::GetPrefs(
    content::BrowserContext* context) const {
  return user_prefs::UserPrefs::Get(context);
}

std::string SubscriptionServiceFactory::GetLocale(PrefService* prefs) const {
  return language::GetApplicationLocale(prefs);
}

}  // namespace adblock
