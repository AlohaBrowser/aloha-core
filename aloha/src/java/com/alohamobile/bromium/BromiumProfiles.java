// Copyright 2026 Aloha Mobile Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
package com.alohamobile.bromium;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;

import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwBrowserContextStore;
import org.chromium.android_webview.AwContents;

/**
 * ALOHA CU-86eqe7rd2: facade for the Kotlin layer over the public/private
 * profile model (replaced the old global "active cookie mode").
 *
 * <p>A mode is a profile assigned to a WebView AT CREATION, before the first
 * navigation ({@link AwContents#setBrowserContextForPublicApi}). The mode of a
 * live WebView cannot be switched — a tab of the new mode is opened under its
 * own profile.
 *
 * <p>Ephemerality of the private mode is the app's responsibility: the
 * "Incognito" profile is persistent, the app wipes it via {@link
 * #clearIncognitoData} (e.g. at startup, before restoring private tabs). All
 * methods are UI-thread only.
 */
@JNINamespace("aloha")
public final class BromiumProfiles {
    /** Public-mode profile name (upstream default context). */
    public static final String DEFAULT = "Default";
    /** Private-mode profile name. Matches kIncognitoContextName. */
    public static final String INCOGNITO = "Incognito";

    private BromiumProfiles() {}

    /** Public-mode profile. */
    @MainThread
    @NonNull
    public static AwBrowserContext getDefaultProfile() {
        // ALOHA async startup: the native context store exists only after the browser
        // process started; flush a pending async startup synchronously if needed.
        BromiumStartupState.ensureStarted();
        return AwBrowserContextStore.getNamedContext(DEFAULT, true);
    }

    /** Private-mode profile (created lazily on first access). */
    @MainThread
    @NonNull
    public static AwBrowserContext getIncognitoProfile() {
        BromiumStartupState.ensureStarted();
        return AwBrowserContextStore.getNamedContext(INCOGNITO, true);
    }

    /** Profile by name (for a future "one-shot" and other modes). */
    @MainThread
    @NonNull
    public static AwBrowserContext getProfile(@NonNull String name) {
        BromiumStartupState.ensureStarted();
        return AwBrowserContextStore.getNamedContext(name, true);
    }

    /**
     * Assign a profile to a freshly created WebView. Must be called before the
     * first navigation / evaluateJavascript / profile read, otherwise
     * {@link AwContents#setBrowserContextForPublicApi} throws
     * IllegalStateException. Tab-restore order:
     * create WebView → setProfile → restoreState.
     */
    @MainThread
    public static void setProfile(@NonNull AwContents awContents, @NonNull String name) {
        awContents.setBrowserContextForPublicApi(getProfile(name));
    }

    /**
     * WebView's profile name without blocking a later setProfile (replacement
     * for the old isPrivateMode). {@code isIncognito(awContents)} is equivalent
     * to the former isPrivateMode.
     */
    @MainThread
    @NonNull
    public static String profileName(@NonNull AwContents awContents) {
        return awContents.getProfileNameForBromium();
    }

    /** true if the WebView is in private mode. */
    @MainThread
    public static boolean isIncognito(@NonNull AwContents awContents) {
        return INCOGNITO.equals(profileName(awContents));
    }

    /**
     * Full data wipe of the private profile: cookies (incl. session),
     * DOM storage, IndexedDB/CacheStorage/FileSystem, HTTP cache, Service
     * Workers, permissions/site settings. Works with open Incognito tabs (they
     * keep running; in-memory document state is not touched). Asynchronous.
     */
    @MainThread
    public static void clearIncognitoData() {
        clearProfileData(INCOGNITO);
    }

    /** Full data wipe of the profile by name. */
    @MainThread
    public static void clearProfileData(@NonNull String name) {
        BromiumStartupState.ensureStarted();
        // Content layer: cookies, storages, cache, SW (BrowsingDataRemover).
        BromiumProfilesJni.get().clearProfileData(name);
        // Site settings/permissions are stored in per-profile SharedPreferences
        // and not covered by BrowsingDataRemover — cleared separately.
        getProfile(name).getGeolocationPermissions().clearAll();
    }

    /** Clear the HTTP cache of the profile by name. */
    @MainThread
    public static void clearProfileCache(@NonNull String name) {
        BromiumStartupState.ensureStarted();
        BromiumProfilesJni.get().clearProfileCache(name);
    }

    /** Clear the Service Worker data of the profile by name. */
    @MainThread
    public static void clearProfileServiceWorkerData(@NonNull String name) {
        BromiumStartupState.ensureStarted();
        BromiumProfilesJni.get().clearServiceWorkerData(name);
    }

    @NativeMethods
    interface Natives {
        void clearProfileData(@JniType("std::string") String name);
        void clearProfileCache(@JniType("std::string") String name);
        void clearServiceWorkerData(@JniType("std::string") String name);
    }
}
