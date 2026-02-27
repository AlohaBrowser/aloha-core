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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEB_UI_CONTROLLER_FACTORY_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEB_UI_CONTROLLER_FACTORY_H_

#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace adblock {

namespace {

const content::WebUI::TypeID kAdblockInternalsID = &kAdblockInternalsID;

}  // namespace

// Owned by WebUIConfigMap. Used to hook up with the existing WebUI infra.
class AdblockWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static AdblockWebUIControllerFactory* GetInstance();

  AdblockWebUIControllerFactory(const AdblockWebUIControllerFactory&) = delete;
  AdblockWebUIControllerFactory& operator=(
      const AdblockWebUIControllerFactory&) = delete;

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

 private:
  friend struct base::DefaultSingletonTraits<AdblockWebUIControllerFactory>;

  AdblockWebUIControllerFactory() = default;
  ~AdblockWebUIControllerFactory() override = default;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEB_UI_CONTROLLER_FACTORY_H_
