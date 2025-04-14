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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_FOR_TEST_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_FOR_TEST_H_

#include "components/adblock/content/browser/adblock_url_loader_factory.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// A simple class which handles following commands passed via intercepted url
// in a format `adblock.test.data/[topic]/[action]/[payload]` where:
// - `topic` is either `domains` (allowed domains), `filters`, `subscriptions`,
//   `adblock`, `aa` (Acceptable Ads)
// - `action` is either:
//    - `add`, `clear` (remove all), `list`, `remove` valid for `domains`,
//      `filters` and `subscriptions`
//    - `enable`, `disable` or `state` valid for `aa` and `adblock`
// - `payload` is url encoded string required for action `add` and `remove`.
// When adding or removing filter/domain/subscription one can encode several
// entries splitting them by a new line character.
class AdblockURLLoaderFactoryForTest final : public AdblockURLLoaderFactory {
 public:
  AdblockURLLoaderFactoryForTest(
      AdblockURLLoaderFactoryConfig config,
      RequestInitiator request_initiator,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      std::string user_agent_string,
      DisconnectCallback on_disconnect,
      content::BrowserContext* context);
  ~AdblockURLLoaderFactoryForTest() final;

  void CreateLoaderAndStart(
      ::mojo::PendingReceiver<::network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const ::network::ResourceRequest& request,
      ::mojo::PendingRemote<::network::mojom::URLLoaderClient> client,
      const ::net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      final;

  static const char kEyeoDebugDataHostName[];

 private:
  std::string HandleCommand(const GURL& url) const;
  std::string HandleConfigurations(const GURL& url) const;
  FilteringConfiguration* GetConfiguration() const;
  void SendResponse(
      std::string response_body,
      ::mojo::PendingRemote<network::mojom::URLLoaderClient> client) const;
  raw_ptr<SubscriptionService> subscription_service_;
  raw_ptr<PrefService> prefs_;
  std::string configuration_name_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_URL_LOADER_FACTORY_FOR_TEST_H_
