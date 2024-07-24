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

#include "components/adblock/content/browser/factories/session_stats_factory.h"

#include <memory>

#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/session_stats_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
SessionStats* SessionStatsFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionStats*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
SessionStatsFactory* SessionStatsFactory::GetInstance() {
  static base::NoDestructor<SessionStatsFactory> instance;
  return instance.get();
}

SessionStatsFactory::SessionStatsFactory()
    : BrowserContextKeyedServiceFactory(
          "SessionStats",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ResourceClassificationRunnerFactory::GetInstance());
}

SessionStatsFactory::~SessionStatsFactory() = default;

KeyedService* SessionStatsFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SessionStatsImpl(
      ResourceClassificationRunnerFactory::GetForBrowserContext(context));
}

content::BrowserContext* SessionStatsFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

}  // namespace adblock
