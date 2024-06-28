// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

class WebViewChromiumFactoryProviderForR extends WebViewChromiumFactoryProvider {
    public static WebViewChromiumFactoryProvider create(com.android.webview.chromium.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProviderForR(delegate);
    }

    protected WebViewChromiumFactoryProviderForR(com.android.webview.chromium.WebViewDelegate delegate) {
        super(delegate);
    }
}
