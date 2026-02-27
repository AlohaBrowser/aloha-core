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

import java.util.Arrays;
import java.util.Objects;

public abstract class BromiumClient {
    public static final class MediaPlayerId {
        public MediaPlayerId(String htmlId, String[] iframesIds,
                             int cid, int rid, int did) {
            this.htmlId = htmlId;
            this.iframesIds = iframesIds;
            this.cid = cid;
            this.rid = rid;
            this.did = did;
        }

        public String htmlId;
        public String[] iframesIds;
        public int cid;
        public int rid;
        public int did;

        @Override
        public boolean equals(Object other) {
            if (other == null) return false;
            if (getClass() != other.getClass()) return false;
            final MediaPlayerId otherId = (MediaPlayerId)other;
            if (this.cid != otherId.cid) return false;
            if (this.rid != otherId.rid) return false;
            if (this.did !=  otherId.did) return false;
            if (!Objects.equals(this.htmlId, otherId.htmlId)) return false;
            if (!Arrays.equals(this.iframesIds, otherId.iframesIds)) return false;
            return true;
        }

        @Override
        public int hashCode() {
            return Objects.hash(htmlId, Arrays.hashCode(iframesIds));
        }
    }

    /**
     * Called when media playback starts.
     *
     * @param playerId player ID for media control
     * @param mediaUrl media url
     * @param documentUrl document url with this media
     * @param durationSec track duration in seconds
     * @param isAudioOnly media is audio track (without video)
     */
    // ALOHA https://app.clickup.com/t/2f2eyt8
    public abstract void onMediaPlay(MediaPlayerId playerId, String mediaUrl, String documentUrl,
                                     double durationSec, boolean isAudioOnly);

    /**
     * Called when media is paused.
     * May not be called when open another page, then will be called onMediaDestroy.
     *
     * @param playerId player ID for media control
     * @param mediaUrl media url
     * @param documentUrl document url with this media
     * @param durationSec track duration in seconds
     * @param isAudioOnly media is audio track (without video)
     */
    // ALOHA https://app.clickup.com/t/2qfa6r7
    public abstract void onMediaPause(MediaPlayerId playerId, String mediaUrl, String documentUrl,
                                      double durationSec, boolean isAudioOnly);

    /**
     * Called when media player is destroyed.
     *
     * @param playerId player ID for media control
     * @param mediaUrl media url
     * @param documentUrl document url with this media
     */
    // ALOHA https://app.clickup.com/t/2qfa6r7
    public abstract void onMediaDestroy(MediaPlayerId playerId, String mediaUrl, String documentUrl);

    /**
     * Called when media player pipeline reports an error.
     *
     * @param playerId player ID
     * @param pipelineStatus pipeline status at time of the error as string
     * @param mediaUrl media url
     * @param documentUrl document url with this media
     * @param currentTimeSec playback position at time of the error in seconds
     * @param durationSec media duration in seconds
     */
    // ALOHA https://app.clickup.com/t/2rqdtxz
    public abstract void onMediaError(MediaPlayerId playerId, String pipelineStatus,
        String mediaUrl, String documentUrl, double currentTimeSec, double durationSec);

    /**
     * ALOHA: subtitles. Called with the full snapshot of the player's text
     * tracks: once when the player is created (if it already has tracks), and
     * then whenever tracks are added/removed, a track's mode changes or a
     * &lt;track&gt; resource finishes loading.
     *
     * <p>Separate from {@link #onMediaPlay} on purpose: at the time playback
     * starts the tracks are usually not loaded yet (HLS/DASH/MSE/script load
     * them asynchronously), so this is called several times for the same
     * playerId. Every call replaces the previous list — indexes inside
     * {@link SubtitleTrack} are only valid until the next call.
     *
     * <p>Not called at all for a player that never has any text track.
     *
     * @param playerId player ID for media control
     * @param tracks current text tracks, may be empty
     */
    public abstract void onTextTracksChanged(MediaPlayerId playerId, SubtitleTrack[] tracks);

    /**
     * ALOHA https://app.clickup.com/t/2558578/z93epac4kc
     * One line per fullscreen video session, for analytics as is (no URL,
     * player name or subtitle text): {@code "<code> <message>"}, message up
     * to 15 characters.
     *
     * <p>Codes: {@code 0} no subtitles; {@code 1} engine draws them
     * ({@code forced} = our fullscreen takeover, {@code dual} = page draws a
     * copy too); {@code 2 known} page draws them in HTML, player known;
     * {@code 3 native|mse} HLS manifest advertises them, nothing drawn;
     * {@code 4 unk} page draws them, player unknown; {@code 5} tracks exist,
     * none enabled; {@code 6} file fetched, nothing shown.
     *
     * <p>Message tokens while they fit: code qualifier, {@code low}
     * (session too short to trust), formats fetched ({@code vtt srt ttml ass
     * sbv json}), source ({@code url blob ms}). Default does nothing.
     */
    public void onSubtitleStat(String stat) {}

    // ALOHA https://app.clickup.com/t/2hxwa9w, https://app.clickup.com/t/861m7a4e2
    public abstract boolean shouldHideControlsInFullscreen(MediaPlayerId playerId, String url, double durationSec, String playerClass);
    // Called only if video (HTMLMediaElement) required fullscreen.
    public abstract void onVideoEnterFullscreen(MediaPlayerId playerId, String url, boolean mediaControlsIsHidden);
    public abstract void onVideoExitFullscreen(MediaPlayerId playerId, String url);
    // ALOHA https://app.clickup.com/t/861m7mewp
    // Called if any content required fullscreen.
    public abstract void onEnterFullscreen();
    public abstract void onExitFullscreen();

    // ALOHA https://app.clickup.com/t/2f2f3we. Will be removed in https://app.clickup.com/t/2qf97ya.
    public abstract void onPageLoaded(String url, boolean isError);

    /**
     * Called when download started and Chromium is handling it
     * @param httpMethod HTTP method GET or POST
     * @param sizeInBytes file size in bytes
     */
    // ALOHA https://app.clickup.com/t/861md9r6t
    public abstract void onInternalDownloadStarted(String httpMethod, int sizeInBytes);

    /**
     * ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h and
     * https://app.clickup.com/t/861me45jv
     * Called for downloads by POST and for request by AwContents.requestDownloadUrl().
     * If file by requestDownloadUrl() not in http cache then AwContents.onDownloadStart()
     * is called instead of this method.
     * @param url URL from AwContentsClient.onDownloadStart() after redirects
     * @param originalUrl the same parameter in AwContentsClient.onDownloadStart()
     * @param userAgent the same parameter in AwContentsClient.onDownloadStart()
     * @param contentDisposition the same parameter in AwContentsClient.onDownloadStart()
     * @param mimeType the same parameter in AwContentsClient.onDownloadStart()
     * @param suggestedFilename suggested filename from chromium-suggester
     * @param downloadedFilePath path to temporary file with downloaded content
     */
    public abstract void onDownloadToCacheFinished(String url, String originalUrl, String userAgent, String contentDisposition,
        String mimeType, String suggestedFilename, String downloadedFilePath);

    // ALOHA https://app.clickup.com/t/2u59j0h
    // For bromium internal use. For calculate state of download.
    public abstract void onRequestedDownloadUrl(String url);

    // ALOHA https://app.clickup.com/t/86epnk66e
    // Called for checking background video settings.
    public abstract boolean shouldPlayBackgroundVideo();
    
    /**
     * ALOHA: https://app.clickup.com/t/86ewr2k2q
     * Called when HLS stream is detected in downloading
     * @param url URL of the HLS stream
     * @param requestHeaders HTTP request headers for the HLS stream in JSON format
     */ 
    public abstract void onHlsDetected(String url, String requestHeaders);

    // ALOHA https://app.clickup.com/t/86ewz0pbr
    public abstract String getUserAgent();

    public abstract void onReinitializeMainRenderFrame(boolean isRenderFrameLive);

}
