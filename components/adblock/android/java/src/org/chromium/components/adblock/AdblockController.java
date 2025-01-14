/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.components.adblock;

import androidx.annotation.UiThread;

import org.jni_zero.CalledByNative;
import org.jni_zero.NativeMethods;

import org.chromium.base.ResettersForTesting;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.content_public.browser.BrowserContextHandle;

import java.net.URL;
import java.util.Arrays;
import java.util.List;

/**
 * DEPRECATED, please use {@link FilteringConfiguration} instead.
 *
 * @brief Main access point for java UI code to control ad filtering. It calls its native counter
 *     part also AdblockController. It lives in UI thread on the browser process.
 */
@Deprecated
public class AdblockController {
    private static final String TAG = AdblockController.class.getSimpleName();
    private FilteringConfiguration mFilteringConfiguration;
    private BrowserContextHandle mBrowserContextHandle;

    private URL mAcceptableAds;

    private static AdblockController sInstance;

    private AdblockController(BrowserContextHandle contextHandle) {
        mBrowserContextHandle = contextHandle;
        mFilteringConfiguration =
                FilteringConfiguration.createConfiguration("adblock", mBrowserContextHandle);
        try {
            mAcceptableAds =
                    new URL("https://easylist-downloads.adblockplus.org/exceptionrules.txt");
        } catch (java.net.MalformedURLException e) {
            mAcceptableAds = null;
        }
    }

    /**
     * @return The singleton object.
     */
    public static AdblockController getInstance(BrowserContextHandle contextHandle) {
        // Make sure native libraries are ready to avoid org.chromium.base.JniException
        LibraryLoader.getInstance().ensureInitialized();
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new AdblockController(contextHandle);
        }
        return sInstance;
    }

    public static void setInstanceForTesting(final AdblockController adblockController) {
        var oldValue = sInstance;
        sInstance = adblockController;
        ResettersForTesting.register(() -> sInstance = oldValue);
    }

    public static class Subscription {
        private URL mUrl;
        private String mTitle;
        private String mVersion = "";
        private String[] mLanguages = {};
        private boolean mAutoInstalled;

        public Subscription(final URL url, final String title, final String version) {
            this.mUrl = url;
            this.mTitle = title;
            this.mVersion = version;
        }

        @CalledByNative("Subscription")
        public Subscription(
                final URL url,
                final String title,
                final String version,
                final String[] languages,
                boolean autoinstalled) {
            this.mUrl = url;
            this.mTitle = title;
            this.mVersion = version;
            this.mLanguages = languages;
            this.mAutoInstalled = autoinstalled;
        }

        public String title() {
            return mTitle;
        }

        public URL url() {
            return mUrl;
        }

        public String version() {
            return mVersion;
        }

        public String[] languages() {
            return mLanguages;
        }

        public boolean autoinstalled() {
            return mAutoInstalled;
        }

        @Override
        public boolean equals(final Object object) {
            if (object == null) return false;
            if (getClass() != object.getClass()) return false;

            Subscription other = (Subscription) object;
            return url().equals(other.url());
        }
    }

    @UiThread
    public void setAcceptableAdsEnabled(boolean enabled) {
        if (enabled) mFilteringConfiguration.addFilterList(mAcceptableAds);
        else mFilteringConfiguration.removeFilterList(mAcceptableAds);
    }

    @UiThread
    public boolean isAcceptableAdsEnabled() {
        return mFilteringConfiguration.getFilterLists().contains(mAcceptableAds);
    }

    @UiThread
    public List<Subscription> getRecommendedSubscriptions() {
        return (List<Subscription>)
                (List<?>) Arrays.asList(AdblockControllerJni.get().getRecommendedSubscriptions());
    }

    @UiThread
    public void setAutoInstallEnabled(boolean enable) {
        FilteringConfiguration.setAutoInstallEnabled(enable, mBrowserContextHandle);
    }

    @UiThread
    public boolean isAutoInstallEnabled() {
        return FilteringConfiguration.isAutoInstallEnabled(mBrowserContextHandle);
    }

    @UiThread
    public void installSubscription(final URL url) {
        mFilteringConfiguration.addFilterList(url);
    }

    @UiThread
    public void uninstallSubscription(final URL url) {
        mFilteringConfiguration.removeFilterList(url);
    }

    @UiThread
    public List<Subscription> getInstalledSubscriptions() {
        return (List<Subscription>)
                (List<?>)
                        Arrays.asList(
                                AdblockControllerJni.get()
                                        .getInstalledSubscriptions(mBrowserContextHandle));
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void setEnabled(boolean enabled) throws IllegalStateException {
        mFilteringConfiguration.setEnabled(enabled);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public boolean isEnabled() throws IllegalStateException {
        return mFilteringConfiguration.isEnabled();
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void removeFilterList(final URL url) throws IllegalStateException {
        mFilteringConfiguration.removeFilterList(url);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public List<URL> getFilterLists() throws IllegalStateException {
        return mFilteringConfiguration.getFilterLists();
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void addAllowedDomain(final String domain) throws IllegalStateException {
        mFilteringConfiguration.addAllowedDomain(domain);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void removeAllowedDomain(final String domain) throws IllegalStateException {
        mFilteringConfiguration.removeAllowedDomain(domain);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public List<String> getAllowedDomains() throws IllegalStateException {
        return mFilteringConfiguration.getAllowedDomains();
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void addCustomFilter(final String filter) throws IllegalStateException {
        mFilteringConfiguration.addCustomFilter(filter);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public void removeCustomFilter(final String filter) throws IllegalStateException {
        mFilteringConfiguration.removeCustomFilter(filter);
    }

    /**
     * @throws IllegalStateException when called on removed FilteringConfiguration.
     */
    @UiThread
    public List<String> getCustomFilters() throws IllegalStateException {
        return mFilteringConfiguration.getCustomFilters();
    }

    public interface SubscriptionUpdateObserver
            extends FilteringConfiguration.SubscriptionUpdateObserver {}

    @UiThread
    public void addSubscriptionUpdateObserver(final SubscriptionUpdateObserver observer) {
        mFilteringConfiguration.addSubscriptionUpdateObserver(observer);
    }

    @UiThread
    public void removeSubscriptionUpdateObserver(final SubscriptionUpdateObserver observer) {
        mFilteringConfiguration.removeSubscriptionUpdateObserver(observer);
    }

    @NativeMethods
    interface Natives {
        Object[] getInstalledSubscriptions(BrowserContextHandle contextHandle);

        Object[] getRecommendedSubscriptions();
    }
}
