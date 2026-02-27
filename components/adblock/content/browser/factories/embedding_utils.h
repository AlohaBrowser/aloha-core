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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_FACTORIES_EMBEDDING_UTILS_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_FACTORIES_EMBEDDING_UTILS_H_

#include "components/adblock/content/browser/factories/element_hider_factory.h"
#include "components/adblock/content/browser/factories/sitekey_storage_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/content/browser/page_view_stats.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace adblock {

// Registers an AdblockWebContentObserver for the given WebContents.
template <typename ObserverClass>
void RegisterAdblockWebContentObserver(
    content::WebContents* web_contents,
    content::BrowserContext* browser_context) {
  ObserverClass::CreateForWebContents(
      web_contents,
      SubscriptionServiceFactory::GetForBrowserContext(browser_context),
      ElementHiderFactory::GetForBrowserContext(browser_context),
      SitekeyStorageFactory::GetForBrowserContext(browser_context),
      std::make_unique<FrameHierarchyBuilder>(), CountNavigationsCallback());
}

// Ensures that all background services are started for the given browser
// context.
void EnsureBackgroundServicesStarted(content::BrowserContext* browser_context);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_FACTORIES_EMBEDDING_UTILS_H_
