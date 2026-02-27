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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTEXT_DATA_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTEXT_DATA_H_

#include <memory>
#include <set>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "components/adblock/content/browser/adblock_url_loader_factory.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

namespace adblock {

// Owns all of the AdblockURLLoaderFactory for a given BrowserContext.
class AdblockContextData : public base::SupportsUserData::Data {
 public:
  AdblockContextData(const AdblockContextData&) = delete;
  AdblockContextData& operator=(const AdblockContextData&) = delete;
  ~AdblockContextData() override;

  static void StartProxying(
      content::BrowserContext* browser_context,
      RequestInitiator initiator,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      const std::string& user_agent,
      bool use_test_loader);

 private:
  AdblockContextData();

  void RemoveProxy(AdblockURLLoaderFactory* proxy);

  std::set<std::unique_ptr<AdblockURLLoaderFactory>, base::UniquePtrComparator>
      proxies_;

  base::WeakPtrFactory<AdblockContextData> weak_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTEXT_DATA_H_
