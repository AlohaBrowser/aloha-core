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

/** A single account returned by an identity provider. */
public class FedCmAccount {
    private final String mId;
    private final String mDisplayIdentifier; // email or phone
    private final String mDisplayName;
    private final String mGivenName;
    /** Non-null in multi-IDP mode: the eTLD+1 of the IDP that owns this account. */
    private final String mSecondaryDescription;
    private final Bitmap mAvatarBitmap;
    /** Circle-cropped avatar with IDP badge, only set in multi-IDP mode. */
    private final Bitmap mBadgedAvatarBitmap;
    private final boolean mIsIdpClaimedSignIn;
    private final boolean mIsBrowserTrustedSignIn;
    private final boolean mIsFilteredOut;
    /** IdentityRequestDialogDisclosureField enum values. */
    private final int[] mDisclosureFields;
    private final FedCmIdentityProviderData mIdentityProvider;

    @CalledByNative
    public FedCmAccount(String id, String displayIdentifier, String displayName,
            String givenName, String secondaryDescription,
            Bitmap avatarBitmap, Bitmap badgedAvatarBitmap,
            boolean isIdpClaimedSignIn, boolean isBrowserTrustedSignIn,
            boolean isFilteredOut, int[] disclosureFields,
            FedCmIdentityProviderData identityProvider) {
        mId = id;
        mDisplayIdentifier = displayIdentifier;
        mDisplayName = displayName;
        mGivenName = givenName;
        mSecondaryDescription = secondaryDescription;
        mAvatarBitmap = avatarBitmap;
        mBadgedAvatarBitmap = badgedAvatarBitmap;
        mIsIdpClaimedSignIn = isIdpClaimedSignIn;
        mIsBrowserTrustedSignIn = isBrowserTrustedSignIn;
        mIsFilteredOut = isFilteredOut;
        mDisclosureFields = disclosureFields;
        mIdentityProvider = identityProvider;
    }

    public String getId() { return mId; }
    public String getDisplayIdentifier() { return mDisplayIdentifier; }
    public String getDisplayName() { return mDisplayName; }
    public String getGivenName() { return mGivenName; }
    public String getSecondaryDescription() { return mSecondaryDescription; }
    public Bitmap getAvatarBitmap() { return mAvatarBitmap; }
    public Bitmap getBadgedAvatarBitmap() { return mBadgedAvatarBitmap; }
    public boolean isIdpClaimedSignIn() { return mIsIdpClaimedSignIn; }
    public boolean isBrowserTrustedSignIn() { return mIsBrowserTrustedSignIn; }
    public boolean isFilteredOut() { return mIsFilteredOut; }
    public int[] getDisclosureFields() { return mDisclosureFields; }
    public FedCmIdentityProviderData getIdentityProvider() { return mIdentityProvider; }
}
