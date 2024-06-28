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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_PAGE_HANDLER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "components/adblock/content/browser/mojom/adblock_internals.mojom.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace adblock {

class AdblockInternalsPageHandler
    : public mojom::adblock_internals::AdblockInternalsPageHandler {
 public:
  explicit AdblockInternalsPageHandler(
      content::BrowserContext* context,
      mojo::PendingReceiver<
          mojom::adblock_internals::AdblockInternalsPageHandler> receiver);
  AdblockInternalsPageHandler(const AdblockInternalsPageHandler&) = delete;
  AdblockInternalsPageHandler& operator=(const AdblockInternalsPageHandler&) =
      delete;
  ~AdblockInternalsPageHandler() override;

  // mojom::adblock_internals::AdblockInternalsPageHandler:
  void GetDebugInfo(GetDebugInfoCallback callback) override;
  void ToggleTestpagesFLSubscription(
      ToggleTestpagesFLSubscriptionCallback callback) override;
  void IsSubscribedToTestpagesFL(
      IsSubscribedToTestpagesFLCallback callback) override;

 private:
  static void OnTelemetryServiceInfoArrived(
      GetDebugInfoCallback callback,
      std::string content,
      std::vector<std::string> topic_provider_content);
  bool IsSubscribedToTestpagesFL() const;

  raw_ptr<content::BrowserContext> context_;
  mojo::Receiver<mojom::adblock_internals::AdblockInternalsPageHandler>
      receiver_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_PAGE_HANDLER_H_
