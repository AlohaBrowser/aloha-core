// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package com.alohamobile.bromium;

import android.content.Context;
import android.os.Build;
import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.common.AwResource;
import org.chromium.android_webview.gfx.AwDrawFnImpl;
import org.chromium.base.ContextUtils;
import org.chromium.ui.base.ResourceBundle;
import com.alohamobile.bromium.BromiumResources;
import com.android.webview.chromium.DrawFunctor;
import com.android.webview.chromium.DrawGLFunctor;

public class BromiumInitializer {
    public static void initializeAll(Context context, Integer configKeySystemUuidMapping) {
        ContextUtils.initApplicationContext(context.getApplicationContext());

        AwResource.setResources(context.getResources());
        AwResource.setConfigKeySystemUuidMapping(configKeySystemUuidMapping);

        BromiumResources.init(context.getApplicationContext());

        System.loadLibrary("webviewchromium_plat_support");
        AwBrowserProcess.loadLibrary(null);

        String[] uncompressLocales = {
            // Default locale:
            "en-US",
            // Locales from aloha-browser-android/app/build.gradle.kts:
            "ar", "de", "el", "es", "fa", "fr", "iw", "hi", "in", "it", "ja", "ko", "ms", "nl", "pt",
            "ru", "sv", "th", "tr", "vi", "zh-TW", "zh-CN", "uk", "cs", "ro", "pl", "hu", "ca", "hr",
            "da", "fi", "no", "sk",
        };
        ResourceBundle.setAvailablePakLocales(uncompressLocales);

        DrawGLFunctor.setChromiumAwDrawGLFunction(AwContents.getAwDrawGLFunction());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            AwDrawFnImpl.setDrawFnFunctionTable(DrawFunctor.getDrawFnFunctionTable());
        }

        AwBrowserProcess.start();

        Bromium.clearTemporaryDownloads();
    }
}
