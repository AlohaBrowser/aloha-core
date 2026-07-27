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

/** Relying party data passed from C++ FedCM engine to UI. */
public class FedCmRelyingPartyData {
    private final String mRpForDisplay;
    private final String mIframeForDisplay;
    private final Bitmap mRpIcon;
    private final boolean mDisplayStringsMayChange;

    @CalledByNative
    public FedCmRelyingPartyData(String rpForDisplay, String iframeForDisplay,
            Bitmap rpIcon, boolean displayStringsMayChange) {
        mRpForDisplay = rpForDisplay;
        mIframeForDisplay = iframeForDisplay;
        mRpIcon = rpIcon;
        mDisplayStringsMayChange = displayStringsMayChange;
    }

    public String getRpForDisplay() { return mRpForDisplay; }
    public String getIframeForDisplay() { return mIframeForDisplay; }
    public Bitmap getRpIcon() { return mRpIcon; }
    public boolean getDisplayStringsMayChange() { return mDisplayStringsMayChange; }
}
