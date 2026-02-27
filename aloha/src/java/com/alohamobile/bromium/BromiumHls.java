// Copyright 2026 Aloha Mobile Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

package com.alohamobile.bromium;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.android_webview.AwContents;
import org.chromium.content_public.browser.WebContents;

/**
 * ALOHA https://app.clickup.com/t/z93epaa335: HLS manifests cached per tab.
 *
 * <p>Complements {@link BromiumClient#onHlsDetected}: while the page's player
 * fetches an HLS manifest, Bromium keeps the complete manifest body for the
 * lifetime of the WebView's WebContents (bounded LRU, see
 * aloha/src/native/hls_manifest_cache.h). The downloader can then reuse it
 * instead of re-requesting it: PH signs playlist URLs per client IP and
 * Referer and is testing single-use ones, so the player's copy is the one
 * that is guaranteed to exist.
 *
 * <p>Keys: the request URL of every manifest the detector saw (master and
 * every media playlist the player loaded), normalized: no fragment, no
 * LL-HLS delivery directives ({@code _HLS_msn}, {@code _HLS_part},
 * {@code _HLS_skip}), no empty query. The rest of the query (signatures)
 * must match byte for byte. Both the pre-redirect and the final URL resolve.
 * A live playlist is a snapshot of the last reload. UI thread only.
 */
@JNINamespace("aloha")
public final class BromiumHls {
    private BromiumHls() {}

    /**
     * Manifest text for {@code url} as the page received it, or {@code null}
     * when it was not seen in this WebView or has already been evicted.
     */
    @MainThread
    @Nullable
    public static String getCachedManifest(@NonNull AwContents awContents, @NonNull String url) {
        WebContents webContents = awContents.getWebContents();
        if (webContents == null) return null;
        return BromiumHlsJni.get().getCachedManifest(webContents, url);
    }

    @NativeMethods
    interface Natives {
        @Nullable
        String getCachedManifest(WebContents webContents, @JniType("std::string") String url);
    }
}
