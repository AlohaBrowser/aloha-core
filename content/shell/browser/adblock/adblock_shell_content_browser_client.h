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

#ifndef CONTENT_SHELL_BROWSER_ADBLOCK_ADBLOCK_SHELL_CONTENT_BROWSER_CLIENT_H_
#define CONTENT_SHELL_BROWSER_ADBLOCK_ADBLOCK_SHELL_CONTENT_BROWSER_CLIENT_H_

#include "components/adblock/content/browser/adblock_content_browser_client.h"
#include "content/shell/browser/shell_content_browser_client.h"

namespace content {

class AdblockShellContentBrowserClient
    : public adblock::AdblockContentBrowserClient<ShellContentBrowserClient> {
 private:
  content::BrowserContext* GetBrowserContextForEyeoFactories(
      content::BrowserContext* current_browser_context) override;
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_ADBLOCK_ADBLOCK_SHELL_CONTENT_BROWSER_CLIENT_H_
