// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import org.chromium.android_webview.common.Lifetime;

@Lifetime.Singleton
class WebViewChromiumFactoryProviderForT extends WebViewChromiumFactoryProvider {
    public static WebViewChromiumFactoryProvider create(com.android.webview.chromium.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProviderForT(delegate);
    }

    protected WebViewChromiumFactoryProviderForT(com.android.webview.chromium.WebViewDelegate delegate) {
        super(delegate);
    }
}
