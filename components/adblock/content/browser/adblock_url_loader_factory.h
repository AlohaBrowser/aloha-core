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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_H_

#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/raw_ptr.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace adblock {

class SubscriptionService;
class ResourceClassificationRunner;
class ElementHider;
class SitekeyStorage;
class ContentSecurityPolicyInjector;

struct AdblockURLLoaderFactoryConfig {
  // These raw_ptrs have DisableDanglingPtrDetection because they point to
  // KeyedServices that will be removed with the BrowserContext. This object has
  // no good method to find out when the BrowserContext is destroyed, so it
  // can't null the pointers.
  raw_ptr<SubscriptionService, DisableDanglingPtrDetection>
      subscription_service = nullptr;
  raw_ptr<ResourceClassificationRunner, DisableDanglingPtrDetection>
      resource_classifier = nullptr;
  raw_ptr<ElementHider, DisableDanglingPtrDetection> element_hider = nullptr;
  raw_ptr<SitekeyStorage, DisableDanglingPtrDetection> sitekey_storage =
      nullptr;
  raw_ptr<ContentSecurityPolicyInjector, DisableDanglingPtrDetection>
      csp_injector = nullptr;
};

// Processing network requests and responses.
class AdblockURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  using DisconnectCallback = base::OnceCallback<void(AdblockURLLoaderFactory*)>;

  AdblockURLLoaderFactory(
      AdblockURLLoaderFactoryConfig config,
      RequestInitiator request_initiator,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      std::string user_agent_string,
      DisconnectCallback on_disconnect);
  ~AdblockURLLoaderFactory() override;

  void CreateLoaderAndStart(
      ::mojo::PendingReceiver<::network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const ::network::ResourceRequest& request,
      ::mojo::PendingRemote<::network::mojom::URLLoaderClient> client,
      const ::net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
  void Clone(::mojo::PendingReceiver<URLLoaderFactory> factory) override;

 private:
  class InProgressRequest;
  friend class InProgressRequest;

  void OnTargetFactoryError();
  void OnProxyBindingError();
  void RemoveRequest(InProgressRequest* request);
  void MaybeDestroySelf();

  AdblockURLLoaderFactoryConfig config_;
  RequestInitiator request_initiator_;
  mojo::ReceiverSet<network::mojom::URLLoaderFactory> proxy_receivers_;
  std::set<std::unique_ptr<InProgressRequest>, base::UniquePtrComparator>
      requests_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_factory_;
  const std::string user_agent_string_;
  DisconnectCallback on_disconnect_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_H_
