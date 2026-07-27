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

package chromium.chrome.browser.adblock;

import org.jni_zero.NativeMethods;

import org.chromium.content_public.browser.GlobalRenderFrameHostId;

/**
 * @brief Helper class to translate AdblockCounters.ResourceInfo.mMainFrameId to TabId.
 */
public final class TabIdTranslator {
    public static int fromRenderFrameHostId(GlobalRenderFrameHostId id) {
        return TabIdTranslatorJni.get().fromRenderFrameHostId(id.childId(), id.frameRoutingId());
    }

    @NativeMethods
    public interface Natives {
        int fromRenderFrameHostId(int renderProcessId, int renderFrameId);
    }
}
