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

package org.chromium.content_shell.adblock;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.content_public.browser.BrowserContextHandle;

@JNINamespace("adblock")
public class ShellBrowserContext implements BrowserContextHandle {
    /** Pointer to the Native-side ShellBrowserContext. */
    private final long mNativeShellBrowserContext;

    public ShellBrowserContext(long nativeShellBrowserContext) {
        mNativeShellBrowserContext = nativeShellBrowserContext;
    }

    private static ShellBrowserContext sInstance;

    public static ShellBrowserContext getDefault() {
        if (sInstance == null) {
            sInstance = ShellBrowserContextJni.get().getDefaultJava();
        }
        return sInstance;
    }

    @Override
    public long getNativeBrowserContextPointer() {
        return mNativeShellBrowserContext;
    }

    @CalledByNative
    public static ShellBrowserContext create(long nativeShellBrowserContext) {
        return new ShellBrowserContext(nativeShellBrowserContext);
    }

    @NativeMethods
    interface Natives {
        ShellBrowserContext getDefaultJava();
    }
}
