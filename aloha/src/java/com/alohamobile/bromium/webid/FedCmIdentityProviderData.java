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

/** Per-IdP data bundle used by ShowAccountsDialog. */
public class FedCmIdentityProviderData {
    private final String mIdpForDisplay;
    private final FedCmIdentityProviderMetadata mIdpMetadata;
    private final FedCmClientIdMetadata mClientMetadata;
    /** blink::mojom::RpContext: 0=signin, 1=signup, 2=use, 3=continue. */
    private final int mRpContext;
    private final int[] mDisclosureFields;
    private final boolean mHasLoginStatusMismatch;

    @CalledByNative
    public FedCmIdentityProviderData(String idpForDisplay,
            FedCmIdentityProviderMetadata idpMetadata,
            FedCmClientIdMetadata clientMetadata,
            int rpContext, int[] disclosureFields,
            boolean hasLoginStatusMismatch) {
        mIdpForDisplay = idpForDisplay;
        mIdpMetadata = idpMetadata;
        mClientMetadata = clientMetadata;
        mRpContext = rpContext;
        mDisclosureFields = disclosureFields;
        mHasLoginStatusMismatch = hasLoginStatusMismatch;
    }

    public String getIdpForDisplay() { return mIdpForDisplay; }
    public FedCmIdentityProviderMetadata getIdpMetadata() { return mIdpMetadata; }
    public FedCmClientIdMetadata getClientMetadata() { return mClientMetadata; }
    public int getRpContext() { return mRpContext; }
    public int[] getDisclosureFields() { return mDisclosureFields; }
    public boolean getHasLoginStatusMismatch() { return mHasLoginStatusMismatch; }
}
