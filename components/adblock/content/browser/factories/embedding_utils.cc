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

#include "components/adblock/content/browser/factories/embedding_utils.h"

#include <memory>

#include "components/adblock/content/browser/adblock_webcontents_observer.h"
#include "components/adblock/content/browser/factories/adblock_telemetry_service_factory.h"
#include "components/adblock/content/browser/factories/content_security_policy_injector_factory.h"
#include "components/adblock/content/browser/factories/element_hider_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/session_stats_factory.h"
#include "components/adblock/content/browser/factories/sitekey_storage_factory.h"
#include "components/adblock/content/browser/factories/subscription_persistent_metadata_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"

namespace adblock {

void EnsureBackgroundServicesStarted(content::BrowserContext* browser_context) {
  ResourceClassificationRunnerFactory::GetForBrowserContext(browser_context);
  AdblockTelemetryServiceFactory::GetForBrowserContext(browser_context);
  SessionStatsFactory::GetForBrowserContext(browser_context);
  SitekeyStorageFactory::GetForBrowserContext(browser_context);
}

}  // namespace adblock
