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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_IMPL_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_IMPL_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/global_routing_id.h"

namespace adblock {

class ElementHiderImpl final : public ElementHider {
 public:
  explicit ElementHiderImpl(SubscriptionService* subscription_service);
  ~ElementHiderImpl() final;

  void ApplyElementHidingEmulationOnPage(
      GURL url,
      std::vector<GURL> frame_hierarchy,
      content::RenderFrameHost* render_frame_host,
      SiteKey sitekey,
      base::OnceCallback<void(const ElemhideInjectionData&)> on_finished) final;

  bool IsElementTypeHideable(ContentType adblock_resource_type) const final;

  void HideBlockedElement(const GURL& url,
                          content::RenderFrameHost* render_frame_host) final;

 private:
  raw_ptr<SubscriptionService> subscription_service_;
  base::WeakPtrFactory<ElementHiderImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_IMPL_H_
