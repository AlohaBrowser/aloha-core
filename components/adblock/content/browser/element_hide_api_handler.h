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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDE_API_HANDLER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDE_API_HANDLER_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "components/adblock/content/common/element_hide_api.mojom.h"
#include "content/public/browser/browser_context.h"

namespace content {
class RenderFrameHost;
}

namespace adblock {

class ElementHideApiHandler : public mojom::ElementHideApi {
 public:
  ElementHideApiHandler(const ElementHideApiHandler&) = delete;
  ElementHideApiHandler& operator=(const ElementHideApiHandler&) = delete;

  ~ElementHideApiHandler() override;

  static void CreateAndBindElementHideApi(
      base::RepeatingCallback<
          content::BrowserContext*(content::BrowserContext*)> context_mapper,
      content::RenderFrameHost* frame_host,
      mojo::PendingReceiver<mojom::ElementHideApi> receiver);

  // mojom::ElementHideApi implementation.
  void LogSelectors(mojom::ElementHideAction action,
                    const std::vector<std::string>& selectors) override;

 private:
  ElementHideApiHandler(base::RepeatingCallback<content::BrowserContext*(
                            content::BrowserContext*)> context_mapper,
                        int render_process_id,
                        int render_frame_id);
  base::RepeatingCallback<content::BrowserContext*(content::BrowserContext*)>
      context_mapper_;
  const int render_process_id_;
  const int render_frame_id_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDE_API_HANDLER_H_
