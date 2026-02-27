// Copyright 2026 Aloha Mobile Ltd.

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

/**
 * ALOHA: subtitles. Per-player subtitle state and commands.
 *
 * <p>The track list is a snapshot pushed from the renderer via
 * {@link BromiumClient#onTextTracksChanged}; feed every new snapshot into
 * {@link #updateTracks}. Exclusivity ("only one subtitle track showing") is
 * driven here, not by Blink.
 *
 * <p>Commands are asynchronous and fire-and-forget: the result shows up as the
 * next {@link BromiumClient#onTextTracksChanged} snapshot, which the renderer
 * sends even when it rejected the request. Drive the UI off the snapshots, not
 * off the return value of {@link #enable}.
 *
 * <p>Call {@link #invalidate()} once the player is gone (onMediaDestroy) —
 * after that every command is a no-op.
 *
 * <p>Not thread-safe: use it from the UI thread, the same one the callbacks
 * arrive on.
 */
public class SubtitleController {
    /** Values match the ordinals of media::mojom::TextTrackMode. */
    public static final int MODE_DISABLED = SubtitleTrack.MODE_DISABLED;
    public static final int MODE_HIDDEN = SubtitleTrack.MODE_HIDDEN;
    public static final int MODE_SHOWING = SubtitleTrack.MODE_SHOWING;

    private static final SubtitleTrack[] NO_TRACKS = new SubtitleTrack[0];

    private TextTrackCommandTarget mTarget;
    private final BromiumClient.MediaPlayerId mPlayerId;
    private SubtitleTrack[] mTracks;

    public SubtitleController(TextTrackCommandTarget target,
            BromiumClient.MediaPlayerId playerId, SubtitleTrack[] tracks) {
        mTarget = target;
        mPlayerId = playerId;
        mTracks = tracks == null ? NO_TRACKS : tracks;
    }

    public BromiumClient.MediaPlayerId getPlayerId() {
        return mPlayerId;
    }

    /** Last snapshot reported by the renderer. */
    public SubtitleTrack[] getTracks() {
        return mTracks;
    }

    /** The first track with mode "showing", or null when subtitles are off. */
    public SubtitleTrack getActiveTrack() {
        for (SubtitleTrack track : mTracks) {
            if (track != null && track.isEnabled()) return track;
        }
        return null;
    }

    /** Tracks worth showing in a picker: kind subtitles/captions, renderable. */
    public SubtitleTrack[] getSelectableTracks() {
        int count = 0;
        for (SubtitleTrack track : mTracks) {
            if (isSelectable(track)) count++;
        }
        SubtitleTrack[] result = new SubtitleTrack[count];
        int i = 0;
        for (SubtitleTrack track : mTracks) {
            if (isSelectable(track)) result[i++] = track;
        }
        return result;
    }

    private static boolean isSelectable(SubtitleTrack track) {
        return track != null && track.isSubtitleKind() && track.canBeRendered();
    }

    /** Replaces the snapshot; call on every onTextTracksChanged. */
    public void updateTracks(SubtitleTrack[] tracks) {
        mTracks = tracks == null ? NO_TRACKS : tracks;
    }

    /**
     * Index of |track| in the current snapshot, or -1 when it is gone.
     *
     * <p>The renderer keys commands by position in HTMLMediaElement.textTracks,
     * and that position shifts whenever a track is inserted or removed — which
     * happens asynchronously on HLS/DASH pages. So never send the index carried
     * by a track object the caller has been holding on to; look it up in the
     * latest snapshot instead.
     */
    private int currentIndexOf(SubtitleTrack track) {
        for (SubtitleTrack candidate : mTracks) {
            if (candidate == track) return candidate.getIndex();
        }
        for (SubtitleTrack candidate : mTracks) {
            if (candidate != null && candidate.isSameTrack(track)) {
                return candidate.getIndex();
            }
        }
        return -1;
    }

    /**
     * Shows |track| and disables every other currently showing track.
     *
     * @return false when the track is no longer part of the player
     */
    public boolean enable(SubtitleTrack track) {
        if (mTarget == null || track == null) return false;
        int index = currentIndexOf(track);
        if (index < 0) return false;
        for (SubtitleTrack other : mTracks) {
            if (other == null || other.getIndex() == index) continue;
            if (other.isEnabled()) {
                // MODE_DISABLED rather than MODE_HIDDEN on purpose. A player
                // that renders subtitles itself reads them off a hidden track,
                // so leaving the old track hidden means it keeps drawing the
                // previous language over the one just picked -- the switch
                // looks like it did nothing. Disabling silences that renderer.
                // The state is not lost: HTMLMediaElement::RestoreSubtitlesMode
                // puts the track the engine took over back to hidden when
                // fullscreen ends.
                mTarget.setTextTrackMode(mPlayerId, other.getIndex(), MODE_DISABLED);
            }
        }
        mTarget.setTextTrackMode(mPlayerId, index, MODE_SHOWING);
        return true;
    }

    /** @return false when the track is no longer part of the player */
    public boolean disable(SubtitleTrack track) {
        if (mTarget == null || track == null) return false;
        int index = currentIndexOf(track);
        if (index < 0) return false;
        mTarget.setTextTrackMode(mPlayerId, index, MODE_DISABLED);
        return true;
    }

    /** Turns subtitles off completely. */
    public void disableAll() {
        if (mTarget == null) return;
        for (SubtitleTrack track : mTracks) {
            if (track == null || !track.isEnabled()) continue;
            mTarget.setTextTrackMode(mPlayerId, track.getIndex(), MODE_DISABLED);
        }
    }

    /** Detaches from the player; all subsequent commands become no-ops. */
    public void invalidate() {
        mTarget = null;
    }
}
