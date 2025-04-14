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

#include "components/adblock/content/browser/factories/element_hider_factory.h"

#include <memory>

#include "components/adblock/content/browser/element_hider_impl.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
ElementHider* ElementHiderFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ElementHider*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
ElementHiderFactory* ElementHiderFactory::GetInstance() {
  static base::NoDestructor<ElementHiderFactory> instance;
  return instance.get();
}

ElementHiderFactory::ElementHiderFactory()
    : BrowserContextKeyedServiceFactory(
          "ElementHider",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(adblock::SubscriptionServiceFactory::GetInstance());
}

ElementHiderFactory::~ElementHiderFactory() = default;

std::unique_ptr<KeyedService>
ElementHiderFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<ElementHiderImpl>(
      adblock::SubscriptionServiceFactory::GetForBrowserContext(context));
}

content::BrowserContext* ElementHiderFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

}  // namespace adblock
