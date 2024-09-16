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

#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"

#include <memory>

#include "components/adblock/content/browser/factories/sitekey_storage_factory.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/content/browser/resource_classification_runner_impl.h"
#include "components/adblock/core/classifier/resource_classifier_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
ResourceClassificationRunner*
ResourceClassificationRunnerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ResourceClassificationRunner*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
ResourceClassificationRunnerFactory*
ResourceClassificationRunnerFactory::GetInstance() {
  static base::NoDestructor<ResourceClassificationRunnerFactory> instance;
  return instance.get();
}

ResourceClassificationRunnerFactory::ResourceClassificationRunnerFactory()
    : BrowserContextKeyedServiceFactory(
          "ResourceClassificationRunner",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SitekeyStorageFactory::GetInstance());
}

ResourceClassificationRunnerFactory::~ResourceClassificationRunnerFactory() =
    default;

KeyedService* ResourceClassificationRunnerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ResourceClassificationRunnerImpl(
      base::MakeRefCounted<ResourceClassifierImpl>(),
      std::make_unique<FrameHierarchyBuilder>(),
      SitekeyStorageFactory::GetForBrowserContext(context));
}

content::BrowserContext*
ResourceClassificationRunnerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

}  // namespace adblock
