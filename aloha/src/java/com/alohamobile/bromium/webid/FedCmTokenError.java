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

import org.jni_zero.CalledByNative;

/**
 * Error details from an IdP token response. Shown in ShowErrorDialog.
 *
 * <p>Both {@link #getCode()} and {@link #getUrl()} return an empty string when
 * the IdP did not provide error details. UI code should test for non-blank
 * values rather than null:
 * <pre>
 *   if (!tokenError.getCode().isEmpty()) { ... show code ... }
 *   if (!tokenError.getUrl().isEmpty())  { ... show "More details" link ... }
 * </pre>
 */
public class FedCmTokenError {
    private final String mCode;
    private final String mUrl;

    @CalledByNative
    public FedCmTokenError(String code, String url) {
        mCode = code;
        mUrl = url;
    }

    public String getCode() { return mCode; }
    public String getUrl() { return mUrl; }
}
