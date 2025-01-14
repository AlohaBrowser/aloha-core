// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

import {getRequiredElement} from 'chrome://resources/js/util.js';
import {AdblockInternalsPageHandler} from './adblock_internals.mojom-webui.js';

async function debugInfo(): Promise<string> {
  const info = await AdblockInternalsPageHandler.getRemote().getDebugInfo();
  return info.debugInfo;
}

async function refresh() {
  getRequiredElement('content').innerText = await debugInfo();
}

async function toggleTestpagesFLSubscription() {
  const info = await AdblockInternalsPageHandler.getRemote().toggleTestpagesFLSubscription();
  if (info.isSubscribed) {
    getRequiredElement('toggle-tp-fl-button').innerText = "Unsubscribe";
  } else {
    getRequiredElement('toggle-tp-fl-button').innerText = "Subscribe";
  }
  await refresh();
}

async function initializeTestpagesFLSubscriptionButtonText() {
  const info = await AdblockInternalsPageHandler.getRemote().isSubscribedToTestpagesFL();
  if (info.isSubscribed) {
    getRequiredElement('toggle-tp-fl-button').innerText = "Unsubscribe";
  } else {
    getRequiredElement('toggle-tp-fl-button').innerText = "Subscribe";
  }
}

getRequiredElement('copy-button').addEventListener('click', async () => {
  navigator.clipboard.writeText(await debugInfo());
});

getRequiredElement('download-button').addEventListener('click', async () => {
  const url = URL.createObjectURL(new Blob([await debugInfo()], {type: 'text/plain'}));
  const a = document.createElement('a');
  a.href = url;
  a.download = 'adblock-internals.txt';
  a.click();
  URL.revokeObjectURL(url);
});

getRequiredElement('refresh').addEventListener('click', refresh);

getRequiredElement('toggle-tp-fl-button').addEventListener('click', toggleTestpagesFLSubscription);

document.addEventListener('DOMContentLoaded', refresh);
document.addEventListener('DOMContentLoaded', initializeTestpagesFLSubscriptionButtonText);
