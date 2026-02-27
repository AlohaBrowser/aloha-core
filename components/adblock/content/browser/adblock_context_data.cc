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

#include "components/adblock/content/browser/adblock_context_data.h"

#include "components/adblock/content/browser/factories/content_security_policy_injector_factory.h"
#include "components/adblock/content/browser/factories/element_hider_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/sitekey_storage_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/request_initiator.h"

#ifdef EYEO_INTERCEPT_DEBUG_URL
#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#endif

namespace adblock {

AdblockContextData::AdblockContextData() = default;
AdblockContextData::~AdblockContextData() = default;

// static
void AdblockContextData::StartProxying(
    content::BrowserContext* browser_context,
    RequestInitiator initiator,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    const std::string& user_agent,
    bool use_test_loader) {
  const void* const kAdblockContextUserDataKey = &kAdblockContextUserDataKey;
  auto* self = static_cast<AdblockContextData*>(
      browser_context->GetUserData(kAdblockContextUserDataKey));
  if (!self) {
    self = new AdblockContextData();
    browser_context->SetUserData(kAdblockContextUserDataKey,
                                 base::WrapUnique(self));
  }
  adblock::AdblockURLLoaderFactoryConfig config{
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          browser_context),
      adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
          browser_context),
      adblock::ElementHiderFactory::GetForBrowserContext(browser_context),
      adblock::SitekeyStorageFactory::GetForBrowserContext(browser_context),
      adblock::ContentSecurityPolicyInjectorFactory::GetForBrowserContext(
          browser_context)};
#ifdef EYEO_INTERCEPT_DEBUG_URL
  if (use_test_loader) {
    auto proxy = std::make_unique<adblock::AdblockURLLoaderFactoryForTest>(
        std::move(config), std::move(initiator), std::move(receiver),
        std::move(target_factory), user_agent,
        base::BindOnce(&AdblockContextData::RemoveProxy,
                       self->weak_factory_.GetWeakPtr()),
        browser_context);
    self->proxies_.emplace(std::move(proxy));
    return;
  }
#endif
  auto proxy = std::make_unique<adblock::AdblockURLLoaderFactory>(
      std::move(config), std::move(initiator), std::move(receiver),
      std::move(target_factory), user_agent,
      base::BindOnce(&AdblockContextData::RemoveProxy,
                     self->weak_factory_.GetWeakPtr()));
  self->proxies_.emplace(std::move(proxy));
}

void AdblockContextData::RemoveProxy(AdblockURLLoaderFactory* proxy) {
  auto it = proxies_.find(proxy);
  DCHECK(it != proxies_.end());
  proxies_.erase(it);
}

}  // namespace adblock
