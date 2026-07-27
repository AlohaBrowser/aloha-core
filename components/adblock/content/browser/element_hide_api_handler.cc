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

// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/adblock/content/browser/element_hide_api_handler.h"

#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/resource_classification_runner_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "url/gurl.h"

namespace adblock {

ElementHideApiHandler::ElementHideApiHandler(
    base::RepeatingCallback<content::BrowserContext*(content::BrowserContext*)>
        context_mapper,
    int render_process_id,
    int render_frame_id)
    : context_mapper_(context_mapper),
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id) {}

ElementHideApiHandler::~ElementHideApiHandler() = default;

// static
void ElementHideApiHandler::CreateAndBindElementHideApi(
    base::RepeatingCallback<content::BrowserContext*(content::BrowserContext*)>
        context_mapper,
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<mojom::ElementHideApi> receiver) {
  int render_process_id = frame_host->GetProcess()->GetDeprecatedID();
  int render_frame_id = frame_host->GetRoutingID();
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new ElementHideApiHandler(
          context_mapper, render_process_id, render_frame_id)),
      std::move(receiver));
}

void ElementHideApiHandler::LogSelectors(
    mojom::ElementHideAction action,
    const std::vector<std::string>& selectors) {
  auto* rfh =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  DCHECK(rfh);
  if (!rfh) {
    // The frame has been destroyed, so we can't notify observers.
    return;
  }
  auto* rcr = static_cast<ResourceClassificationRunnerImpl*>(
      ResourceClassificationRunnerFactory::GetForBrowserContext(
          context_mapper_.Run(rfh->GetBrowserContext())));
  DCHECK(rcr);
  for (const auto& selector : selectors) {
    rcr->NotifyPageElementMatched(selector,
                                  static_cast<ElementHideAction>(action), rfh);
  }
}

}  // namespace adblock
