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

import org.jni_zero.CalledByNative;

import java.util.Objects;

/**
 * ALOHA: subtitles. A single text track (subtitles/captions) of a media player.
 * Mirrors media::mojom::TextTrackInfo.
 */
public class SubtitleTrack {
    /** TextTrack.mode "disabled" — the track is off. */
    public static final int MODE_DISABLED = 0;
    /**
     * TextTrack.mode "hidden" — the page receives cues but the engine does not
     * draw them. Players that render subtitles themselves (hls.js, shaka) keep
     * the user-selected track in this mode.
     */
    public static final int MODE_HIDDEN = 1;
    /** TextTrack.mode "showing" — the engine draws the cues. */
    public static final int MODE_SHOWING = 2;

    /** Kinds that can actually be displayed as subtitles. */
    public static final String KIND_SUBTITLES = "subtitles";
    public static final String KIND_CAPTIONS = "captions";
    public static final String KIND_DESCRIPTIONS = "descriptions";
    public static final String KIND_CHAPTERS = "chapters";
    public static final String KIND_METADATA = "metadata";

    /**
     * Position in HTMLMediaElement.textTracks at the moment of the snapshot —
     * the key used for commands. NOT stable across snapshots.
     */
    private final int mIndex;
    /** TextTrack.id, may be empty. */
    private final String mId;
    /** TextTrack.label, may be empty. */
    private final String mLabel;
    /** BCP-47 language tag, may be empty. */
    private final String mLanguage;
    /** One of the KIND_* values. */
    private final String mKind;
    /** One of the MODE_* values. */
    private final int mMode;
    /** False for chapters/metadata kinds and for tracks that failed to load. */
    private final boolean mCanBeRendered;

    @CalledByNative
    public SubtitleTrack(int index, String id, String label, String language,
            String kind, int mode, boolean canBeRendered) {
        mIndex = index;
        mId = id;
        mLabel = label;
        mLanguage = language;
        mKind = kind;
        mMode = mode;
        mCanBeRendered = canBeRendered;
    }

    public int getIndex() { return mIndex; }
    public String getId() { return mId; }
    public String getLabel() { return mLabel; }
    public String getLanguage() { return mLanguage; }
    public String getKind() { return mKind; }
    public int getMode() { return mMode; }

    /**
     * True when the engine currently draws this track. A track the page renders
     * on its own is {@link #MODE_HIDDEN} and reports false here — check
     * {@link #getMode()} if that distinction matters.
     */
    public boolean isEnabled() { return mMode == MODE_SHOWING; }

    /**
     * False when asking for {@link #MODE_SHOWING} would be a no-op: kinds
     * "chapters"/"metadata", or a &lt;track&gt; whose resource failed to load.
     */
    public boolean canBeRendered() { return mCanBeRendered; }

    /** True for the kinds worth offering in a subtitle picker. */
    public boolean isSubtitleKind() {
        return KIND_SUBTITLES.equals(mKind) || KIND_CAPTIONS.equals(mKind);
    }

    /**
     * Identity across snapshots. {@link #getIndex()} and {@link #getMode()} are
     * deliberately excluded: both change while the track stays the same one.
     */
    public boolean isSameTrack(SubtitleTrack other) {
        if (other == null) return false;
        return Objects.equals(mId, other.mId)
                && Objects.equals(mLabel, other.mLabel)
                && Objects.equals(mLanguage, other.mLanguage)
                && Objects.equals(mKind, other.mKind);
    }
}
