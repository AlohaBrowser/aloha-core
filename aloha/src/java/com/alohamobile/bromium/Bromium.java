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

import android.util.Size;
import androidx.annotation.AnyThread;
import androidx.annotation.MainThread;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Callback;
import org.chromium.android_webview.AwContents;
import com.alohamobile.bromium.BromiumClient.MediaPlayerId;

@JNINamespace("aloha")
public class Bromium {
    public static final String version = "135.0.7049.38";
    public static final String majorVersion = "135";

    public static void setJavaCallstackFilename(String filename) {
        BromiumJni.get().setJavaCallstackFilename(filename);
    }

    /**
     * ALOHA https://app.clickup.com/t/2f29z75
     */
    public static void setSendDNTHeader(boolean send) {
        BromiumJni.get().setSendDNTHeader(send);
    }

    /**
     * ALOHA https://app.clickup.com/t/2hcppgv
     * Clear public local storage. Session storage will not cleared.
     * Work is run asynchronously.
     */
    @MainThread
    public static void clearPublicLocalStorage() {
        BromiumJni.get().clearLocalStorage(false);
    }

    /**
     * ALOHA https://app.clickup.com/t/2hcppgv
     * Clear private local storage. Session storage will not cleared.
     * Work is run asynchronously.
     */
    @MainThread
    public static void clearPrivateLocalStorage() {
        BromiumJni.get().clearLocalStorage(true);
    }

    /**
     * ALOHA https://app.clickup.com/t/2hcppgv
     * Clear session storage and all local storages.
     * Work is run asynchronously.
     */
    @MainThread
    public static void clearSessionAndLocalStorages() {
        BromiumJni.get().clearSessionStorage();
        clearPrivateLocalStorage();
        clearPublicLocalStorage();
    }

    // ALOHA https://app.clickup.com/t/2tzcywg
    @MainThread
    public static void addIgnoredDcheck(String ignoreSubstring) {
        BromiumJni.get().addIgnoredDcheck(ignoreSubstring);
    }

    /**
     * ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
     * Remove temporary directory with files downloaded by POST or by AwContents.requestDownloadUrl().
     */
    @AnyThread
    public static void clearTemporaryDownloads() {
        BromiumJni.get().clearTemporaryDownloads();
    }

    // ALOHA https://app.clickup.com/t/86enm8q2x
    @AnyThread
    public static String getSignature(String message) {
        return BromiumJni.get().getSignature(message);
    }

    private static String getJSforMedia(MediaPlayerId playerId) {
        String iframesText = "[]";
        if (playerId.iframesIds.length > 0) {
            iframesText = String.format("[ \"%s\" ]", String.join("\", \"", playerId.iframesIds));
        }
        return String.format(
            "elem = findMediaElement(\"%s\", %s);\n" +
            "if (elem == null) null;\n" +
            "else elem.", playerId.htmlId, iframesText);
    }

    public static void mediaPlay(AwContents awContents, MediaPlayerId playerId) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "play()", null);
    }

    public static void mediaPause(AwContents awContents, MediaPlayerId playerId) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "pause()", null);
    }

    public static void mediaSeekToPos(AwContents awContents, MediaPlayerId playerId, double positionSec) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "currentTime=" + positionSec, null);
    }

    private static Double parseDoubleOrNull(String value) {
        if (value == null) {
            return null;
        }
        try {
            return Double.parseDouble(value);
        } catch (NumberFormatException ex) {
            return null;
        }
    }

    private static Integer parseIntOrNull(String value) {
        if (value == null) {
            return null;
        }
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException ex) {
            return null;
        }
    }

    private static Boolean parseBooleanOrNull(String value) {
        if (value == null) {
            return null;
        }
        try {
            return Boolean.parseBoolean(value);
        } catch (NumberFormatException ex) {
            return null;
        }
    }

    // return position in seconds.
    public static void mediaGetCurrentPos(AwContents awContents, MediaPlayerId playerId, Callback<Double> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "currentTime",
            (value) -> { callback.onResult(parseDoubleOrNull(value)); }
        );
    }

    // return duration in seconds.
    public static void mediaGetDuration(AwContents awContents, MediaPlayerId playerId, Callback<Double> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "duration",
            (value) -> { callback.onResult(parseDoubleOrNull(value)); }
        );
    }

    public static void mediaSetPlaybackRate(AwContents awContents, MediaPlayerId playerId, double rate) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "playbackRate=" + rate, null);
    }

    public static void mediaGetPlaybackRate(AwContents awContents, MediaPlayerId playerId, Callback<Double> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "playbackRate",
            (value) -> { callback.onResult(parseDoubleOrNull(value)); }
        );
    }

    public static void mediaCheckPlayer(AwContents awContents, MediaPlayerId playerId, Callback<String> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "tagName",
            (value) -> { callback.onResult(value); }
        );
    }

    public static void mediaGetVideoStreamSize(AwContents awContents, MediaPlayerId playerId, Callback<Size> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "videoHeight",
            (heightStr) -> {
                awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "videoWidth",
                    (widthStr) -> {
                        Integer width = parseIntOrNull(widthStr);
                        Integer height = parseIntOrNull(heightStr);
                        Size result = width != null && height != null ? new Size(width, height) : null;
                        callback.onResult(result);
                    });
            });
    }

    public static void mediaIsPaused(AwContents awContents, MediaPlayerId playerId, Callback<Boolean> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "paused",
            (value) -> { callback.onResult(parseBooleanOrNull(value)); }
        );
    }

    // ALOHA https://app.clickup.com/t/2v1qh1q
    public static void mediaIsMuted(AwContents awContents, MediaPlayerId playerId, Callback<Boolean> callback) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "muted",
            (value) -> { callback.onResult(parseBooleanOrNull(value)); }
        );
    }

    // ALOHA https://app.clickup.com/t/2v1qh1q
    public static void mediaSetMuted(AwContents awContents, MediaPlayerId playerId, boolean muted) {
        awContents.evaluateJavaScriptUnchecked(getJSforMedia(playerId) + "muted=" + muted, null);
    }

    // ALOHA https://app.clickup.com/t/861mawmth
    public static void mediaPlayerNativePlay(AwContents awContents, MediaPlayerId playerId) {
        awContents.mediaPlayerPlayImpl(playerId);
    }

    // ALOHA https://app.clickup.com/t/861mawmth
    public static void mediaPlayerNativePause(AwContents awContents, MediaPlayerId playerId) {
        awContents.mediaPlayerPauseImpl(playerId);
    }

    @NativeMethods
    interface Natives {
        void setJavaCallstackFilename(String fileName);

        // ALOHA https://app.clickup.com/t/2f29z75
        void setSendDNTHeader(boolean send);

        // ALOHA https://app.clickup.com/t/2hcppgv
        void clearSessionStorage();
        void clearLocalStorage(boolean forPrivateMode);

        // ALOHA https://app.clickup.com/t/2tzcywg
        void addIgnoredDcheck(String ignoreSubstring);

        // ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
        void clearTemporaryDownloads();
        
        // ALOHA https://app.clickup.com/t/86enm8q2x
        String getSignature(String message);
    }
}
