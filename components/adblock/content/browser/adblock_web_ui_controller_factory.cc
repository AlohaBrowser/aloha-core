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

#include "components/adblock/content/browser/adblock_web_ui_controller_factory.h"

#include "components/adblock/content/browser/adblock_internals_ui.h"
#include "components/adblock/core/common/web_ui_constants.h"
#include "content/public/common/url_utils.h"

namespace adblock {

// static
AdblockWebUIControllerFactory* AdblockWebUIControllerFactory::GetInstance() {
  return base::Singleton<AdblockWebUIControllerFactory>::get();
}

content::WebUI::TypeID AdblockWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (!content::HasWebUIScheme(url)) {
    return content::WebUI::kNoWebUI;
  }

  if (url.host() == adblock::kChromeUIAdblockInternalsHost) {
    return kAdblockInternalsID;
  }

  return content::WebUI::kNoWebUI;
}

bool AdblockWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

std::unique_ptr<content::WebUIController>
AdblockWebUIControllerFactory::CreateWebUIControllerForURL(
    content::WebUI* web_ui,
    const GURL& url) {
  if (url.host() == adblock::kChromeUIAdblockInternalsHost) {
    return std::make_unique<adblock::AdblockInternalsUI>(web_ui);
  }

  return nullptr;
}

}  // namespace adblock
