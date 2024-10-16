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

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import com.alohamobile.bromium.BromiumClient;
import com.alohamobile.bromium.BromiumClient.MediaPlayerId;

@JNINamespace("aloha")
public abstract class BromiumClientBridge {
    // ALOHA https://app.clickup.com/t/2f2eyt8
    @CalledByNative
    private void onMediaPlay(String player_html_id, String[] player_iframe_path, int cid, int rid, int did, String media_url,
                             String document_url, double duration_s, boolean is_audio_only) {
        getBromiumClient().onMediaPlay(new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did), media_url,
                                       document_url, duration_s, is_audio_only);
    }

    // ALOHA https://app.clickup.com/t/2qfa6r7
    @CalledByNative
    private void onMediaPause(String player_html_id, String[] player_iframe_path, int cid, int rid, int did, String media_url,
                              String document_url, double duration_s, boolean is_audio_only) {
        getBromiumClient().onMediaPause(new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did), media_url,
                                        document_url, duration_s, is_audio_only);
    }

    // ALOHA https://app.clickup.com/t/2qfa6r7
    @CalledByNative
    private void onMediaDestroy(String player_html_id, String[] player_iframe_path, int cid, int rid, int did, String media_url,
                                String document_url) {
        getBromiumClient().onMediaDestroy(new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did), media_url,
                                          document_url);
    }

    // ALOHA https://app.clickup.com/t/2rqdtxz
    @CalledByNative
    private void onMediaError(String player_html_id, String[] player_iframe_path, int cid, int rid, int did, String pipeline_status,
            String media_url, String document_url, double current_time_s, double duration_s) {
        getBromiumClient().onMediaError(new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did),
            pipeline_status, media_url, document_url, current_time_s, duration_s);
    }

    // ALOHA https://app.clickup.com/t/2hxwa9w, https://app.clickup.com/t/861m7a4e2
    @CalledByNative
    private boolean shouldHideControlsInFullscreen(String player_html_id, String[] player_iframe_path, int cid, int rid, int did,
                                                   String url, double duration_s, String playerClass) {
        return getBromiumClient().shouldHideControlsInFullscreen(
            new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did), url, duration_s, playerClass);
    }

    // ALOHA https://app.clickup.com/t/2hxwa9w
    @CalledByNative
    private void onVideoFullscreenChanged(boolean is_fullscreen, String player_html_id, String[] player_iframe_path, int cid, int rid, int did,
                                     String url, boolean media_controls_is_hidden) {        
        MediaPlayerId playerId = new MediaPlayerId(player_html_id, player_iframe_path, cid, rid, did);
        if (is_fullscreen) {
            getBromiumClient().onVideoEnterFullscreen(playerId, url, media_controls_is_hidden);            
        } else {
            getBromiumClient().onVideoExitFullscreen(playerId, url);
        }
    }

    @CalledByNative
    private void onInternalDownloadStarted(String http_method, int size_in_bytes) {
        getBromiumClient().onInternalDownloadStarted(http_method, size_in_bytes);
    }

    // ALOHA https://app.clickup.com/t/86epnk66e
    @CalledByNative
    private boolean shouldPlayBackgroundVideo() {
        return getBromiumClient().shouldPlayBackgroundVideo();
    }

    protected abstract BromiumClient getBromiumClient();
}
