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

#include "components/adblock/content/browser/factories/adblock_telemetry_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "components/adblock/content/browser/factories/adblock_request_throttle_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/page_view_stats.h"
#include "components/adblock/core/activeping_telemetry_topic_provider.h"
#include "components/adblock/core/adblock_telemetry_service.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace adblock {
namespace {

std::optional<base::TimeDelta> g_check_interval_for_testing;

base::TimeDelta GetCheckInterval() {
  static base::TimeDelta kCheckInterval =
      g_check_interval_for_testing ? g_check_interval_for_testing.value()
                                   : base::Minutes(5);
  return kCheckInterval;
}

}  // namespace

// static
AdblockTelemetryService* AdblockTelemetryServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockTelemetryService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
AdblockTelemetryServiceFactory* AdblockTelemetryServiceFactory::GetInstance() {
  static base::NoDestructor<AdblockTelemetryServiceFactory> instance;
  return instance.get();
}

AdblockTelemetryServiceFactory::AdblockTelemetryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockTelemetryService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ResourceClassificationRunnerFactory::GetInstance());
  DependsOn(SubscriptionServiceFactory::GetInstance());
  DependsOn(AdblockRequestThrottleFactory::GetInstance());
}

AdblockTelemetryServiceFactory::~AdblockTelemetryServiceFactory() = default;

content::BrowserContext* AdblockTelemetryServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

KeyedService* AdblockTelemetryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Need to use a URLLoaderFactory specific to the browser context, not from
  // system_network_context_manager(), because the required Accept-Language
  // header depends on user's language settings and is not present in requests
  // made from the System network context.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess();
  auto* subscription_service =
      SubscriptionServiceFactory::GetForBrowserContext(context);
  auto telemetry_service = std::make_unique<AdblockTelemetryService>(
      subscription_service, url_loader_factory,
      AdblockRequestThrottleFactory::GetForBrowserContext(context),
      GetCheckInterval());
  telemetry_service->AddTopicProvider(
      std::make_unique<ActivepingTelemetryTopicProvider>(
          user_prefs::UserPrefs::Get(context), subscription_service,
          ActivepingTelemetryTopicProvider::DefaultBaseUrl(),
          ActivepingTelemetryTopicProvider::DefaultAuthToken(),
          std::make_unique<PageViewStats>(
              ResourceClassificationRunnerFactory::GetForBrowserContext(
                  context),
              user_prefs::UserPrefs::Get(context))));

  if (url_loader_factory) {
    telemetry_service->Start();
  }

  return telemetry_service.release();
}

bool AdblockTelemetryServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void AdblockTelemetryServiceFactory::SetCheckIntervalForTesting(
    base::TimeDelta check_interval) {
  g_check_interval_for_testing = check_interval;
}

}  // namespace adblock
