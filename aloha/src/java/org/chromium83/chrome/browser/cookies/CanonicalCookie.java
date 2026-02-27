// ALOHA https://app.clickup.com/t/2f2f49x
// Copy-paste from 83.0.4103.96.

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Aloha Mobile Ltd.

package org.chromium83.chrome.browser.cookies;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Java representation of net/cookies/canonical_cookie.h.
 *
 * Also has static methods serialize Cookies.
 */
public class CanonicalCookie {
    private final String mName;
    private final String mValue;
    private final String mDomain;
    private final String mPath;
    private final long mCreation;
    private final long mExpiration;
    private final long mLastAccess;
    private final boolean mSecure;
    private final boolean mHttpOnly;
    private final int mSameSite;
    private final int mPriority;
    private final int mSourceScheme;

    /** Constructs a CanonicalCookie */
    public CanonicalCookie(String name, String value, String domain, String path, long creation,
            long expiration, long lastAccess, boolean secure, boolean httpOnly, int sameSite,
            int priority, int sourceScheme) {
        mName = name;
        mValue = value;
        mDomain = domain;
        mPath = path;
        mCreation = creation;
        mExpiration = expiration;
        mLastAccess = lastAccess;
        mSecure = secure;
        mHttpOnly = httpOnly;
        mSameSite = sameSite;
        mPriority = priority;
        mSourceScheme = sourceScheme;
    }

    /** @return Priority of the cookie. */
    public int getPriority() {
        return mPriority;
    }

    /** @return True if the cookie is HTTP only. */
    public boolean isHttpOnly() {
        return mHttpOnly;
    }

    /** @return SameSite enum */
    public int getSameSite() {
        return mSameSite;
    }

    /** @return True if the cookie is secure. */
    public boolean isSecure() {
        return mSecure;
    }

    /** @return Last accessed time. */
    public long getLastAccessDate() {
        return mLastAccess;
    }

    /** @return Expiration time. */
    public long getExpirationDate() {
        return mExpiration;
    }

    /** @return Creation time. */
    public long getCreationDate() {
        return mCreation;
    }

    /** @return Cookie name. */
    public String getName() {
        return mName;
    }

    /** @return Cookie path. */
    public String getPath() {
        return mPath;
    }

    /** @return Cookie domain. */
    public String getDomain() {
        return mDomain;
    }

    /** @return Cookie value. */
    public String getValue() {
        return mValue;
    }

    /** @return Source scheme of the cookie. */
    public int sourceScheme() {
        return mSourceScheme;
    }

    // Note incognito state cannot persist across app installs since the encryption key is stored
    // in the activity state bundle. So the version here is more of a guard than a real version
    // used for format migrations.
    private static final int SERIALIZATION_VERSION = 20191105;
    private static final int UPGRADABLE_SERIALIZATION_VERSION = 20160426;

    public static void saveListToStream(DataOutputStream out, CanonicalCookie[] cookies)
            throws IOException {
        if (out == null) {
            throw new IllegalArgumentException("out arg is null");
        }
        if (cookies == null) {
            throw new IllegalArgumentException("cookies arg is null");
        }
        for (CanonicalCookie cookie : cookies) {
            if (cookie == null) {
                throw new IllegalArgumentException("cookies arg contains null value");
            }
        }

        int length = cookies.length;
        out.writeInt(SERIALIZATION_VERSION);
        out.writeInt(length);
        for (int i = 0; i < length; ++i) {
            cookies[i].saveToStream(out);
        }
    }

    // Not private for tests.
    public static class UnexpectedFormatException extends Exception {
        public UnexpectedFormatException(String message) {
            super(message);
        }
    }

    public static List<CanonicalCookie> readListFromStream(DataInputStream in)
            throws IOException, UnexpectedFormatException {
        if (in == null) {
            throw new IllegalArgumentException("in arg is null");
        }

        boolean unsetSourceScheme = false;
        final int version = in.readInt();
        if (version != SERIALIZATION_VERSION) {
            if (version == UPGRADABLE_SERIALIZATION_VERSION) {
                System.out.println("Cuprum: loading cookies in compatibility mode");
                unsetSourceScheme = true;
            } else {
                throw new UnexpectedFormatException("Unexpected version");
            }
        }
        final int length = in.readInt();
        if (length < 0) {
            throw new UnexpectedFormatException("Negative length: " + length);
        }

        ArrayList<CanonicalCookie> cookies = new ArrayList<>(length);
        for (int i = 0; i < length; ++i) {
            cookies.add(createFromStream(in, unsetSourceScheme));
        }
        return cookies;
    }

    private void saveToStream(DataOutputStream out) throws IOException {
        // URL is no longer included. Keep for backward compatability.
        out.writeUTF("");
        out.writeUTF(mName);
        out.writeUTF(mValue);
        out.writeUTF(mDomain);
        out.writeUTF(mPath);
        out.writeLong(mCreation);
        out.writeLong(mExpiration);
        out.writeLong(mLastAccess);
        out.writeBoolean(mSecure);
        out.writeBoolean(mHttpOnly);
        out.writeInt(mSameSite);
        out.writeInt(mPriority);
        out.writeInt(mSourceScheme);
    }

    private static CanonicalCookie createFromStream(DataInputStream in, boolean unsetSourceScheme) throws IOException {
        // URL is no longer included. Keep for backward compatability.
        in.readUTF();
        return new CanonicalCookie(in.readUTF(), in.readUTF(), in.readUTF(), in.readUTF(),
                in.readLong(), in.readLong(), in.readLong(), in.readBoolean(), in.readBoolean(),
                in.readInt(), in.readInt(),
                unsetSourceScheme? 0: in.readInt());
    }

    @Override
    public String toString() {
        return "Name: " + mName + " Value: " + mValue + " Domain: " + mDomain + " Path: " + mPath + " Creation: " + mCreation + " Expiration: " + mExpiration +
               " LastAccess: " + mLastAccess + " Secure: " + mSecure + " HttpOnly: " + mHttpOnly + " SameSite: " + mSameSite + " Priority: " + mPriority;
    }
}
