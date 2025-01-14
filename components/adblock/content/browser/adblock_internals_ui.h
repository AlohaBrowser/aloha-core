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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_UI_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_UI_H_

#include "components/adblock/content/browser/mojom/adblock_internals.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace adblock {

class AdblockInternalsUI : public ui::MojoWebUIController {
 public:
  explicit AdblockInternalsUI(content::WebUI* web_ui);

  AdblockInternalsUI(const AdblockInternalsUI&) = delete;
  AdblockInternalsUI& operator=(const AdblockInternalsUI&) = delete;

  ~AdblockInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<
          mojom::adblock_internals::AdblockInternalsPageHandler> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();

  std::unique_ptr<mojom::adblock_internals::AdblockInternalsPageHandler>
      handler_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_INTERNALS_UI_H_
