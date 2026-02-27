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

package com.alohamobile.bromium.webid;

import android.graphics.Bitmap;
import org.jni_zero.CalledByNative;

/** Identity provider metadata passed from C++ FedCM engine to UI. */
public class FedCmIdentityProviderMetadata {
    /**
     * Sentinel value meaning "no color override".
     *
     * <p>Mirrors {@code ui::kInvalidJavaColor} on the native side and
     * {@code org.chromium.ui.util.ColorUtils.INVALID_COLOR} in upstream Java.
     * Defined as {@code Integer.MAX_VALUE + 1} (i.e. {@code 0x80000000L}),
     * which is outside the range of valid ARGB int colors, so it cannot
     * collide with any real color value (in particular not with
     * {@link android.graphics.Color#TRANSPARENT}, which is 0).
     *
     * <p>The native helper {@code ui::OptionalSkColorToJavaColor} returns this
     * value when the corresponding {@code std::optional<SkColor>} is empty.
     */
    public static final long INVALID_COLOR = ((long) Integer.MAX_VALUE) + 1;

    private final long mBrandTextColor;
    private final long mBrandBackgroundColor;
    private final Bitmap mBrandIcon;
    private final String mConfigUrl;
    private final String mIdpLoginUrl;
    private final boolean mShowUseDifferentAccountButton;

    @CalledByNative
    public FedCmIdentityProviderMetadata(long brandTextColor, long brandBackgroundColor,
            Bitmap brandIcon, String configUrl, String idpLoginUrl,
            boolean showUseDifferentAccountButton) {
        mBrandTextColor = brandTextColor;
        mBrandBackgroundColor = brandBackgroundColor;
        mBrandIcon = brandIcon;
        mConfigUrl = configUrl;
        mIdpLoginUrl = idpLoginUrl;
        mShowUseDifferentAccountButton = showUseDifferentAccountButton;
    }

    public long getBrandTextColor() { return mBrandTextColor; }
    public long getBrandBackgroundColor() { return mBrandBackgroundColor; }
    public Bitmap getBrandIcon() { return mBrandIcon; }
    public String getConfigUrl() { return mConfigUrl; }
    public String getIdpLoginUrl() { return mIdpLoginUrl; }
    public boolean getShowUseDifferentAccountButton() { return mShowUseDifferentAccountButton; }
}
