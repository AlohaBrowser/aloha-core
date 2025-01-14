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

import org.junit.Assert;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.WebContents;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public abstract class TestPagesHelperBase
        implements FilteringConfiguration.ConfigurationChangeObserver {
    public static final String TESTPAGES_DOMAIN = "abptestpages.org";
    public static final String TESTPAGES_BASE_URL = "https://" + TESTPAGES_DOMAIN;
    public static final String TESTPAGES_TESTCASES_ROOT = TESTPAGES_BASE_URL + "/en/";
    public static final String FILTER_TESTPAGES_TESTCASES_ROOT =
            TESTPAGES_TESTCASES_ROOT + "filters/";
    public static final String EXCEPTION_TESTPAGES_TESTCASES_ROOT =
            TESTPAGES_TESTCASES_ROOT + "exceptions/";
    public static final String CIRCUMVENTION_TESTPAGES_TESTCASES_ROOT =
            TESTPAGES_TESTCASES_ROOT + "circumvention/";
    public static final String SITEKEY_TESTPAGES_TESTCASES_ROOT =
            EXCEPTION_TESTPAGES_TESTCASES_ROOT + "sitekey_mv2";
    public static final String SNIPPETS_TESTPAGES_TESTCASES_ROOT =
            TESTPAGES_TESTCASES_ROOT + "snippets/";
    public static final String TESTPAGES_RESOURCES_ROOT = TESTPAGES_BASE_URL + "/testfiles/";
    public static final String TESTPAGES_SUBSCRIPTION =
            TESTPAGES_TESTCASES_ROOT + "abp-testcase-subscription.txt";
    public static final int TEST_TIMEOUT_SEC = 30;

    private FilteringConfiguration mAdblockFilteringConfiguration;
    private ResourceClassificationNotifier mResourceClassificationNotifier;

    private URL mTestSubscriptionUrl;
    private final CallbackHelper mHelper = new CallbackHelper();
    private final TestResourceFilteringObserver mObserver = new TestResourceFilteringObserver();
    private final TestSubscriptionUpdatedObserver mSubscriptionUpdateObserver =
            new TestSubscriptionUpdatedObserver();

    private class TestSubscriptionUpdatedObserver
            implements FilteringConfiguration.SubscriptionUpdateObserver {
        @Override
        public void onSubscriptionDownloaded(final URL url) {
            if (mTestSubscriptionUrl == null) return;
            if (url.toString().contains(mTestSubscriptionUrl.toString())) {
                Log.d(
                        "TestSubscriptionUpdatedObserver",
                        "Notify subscription updated: " + url.toString());
                mHelper.notifyCalled();
            }
        }
    }

    public void setUp() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mResourceClassificationNotifier =
                            ResourceClassificationNotifier.getInstance(getBrowserContext());
                    mResourceClassificationNotifier.addResourceFilteringObserver(mObserver);
                    // Remove "adblock" configuration to simply drop all default filter lists
                    FilteringConfiguration.removeConfiguration("adblock", getBrowserContext());
                    mAdblockFilteringConfiguration =
                            FilteringConfiguration.createConfiguration(
                                    "adblock", getBrowserContext());
                    Assert.assertEquals(0, mAdblockFilteringConfiguration.getFilterLists().size());
                    mAdblockFilteringConfiguration.addObserver(this);
                    mAdblockFilteringConfiguration.addSubscriptionUpdateObserver(
                            mSubscriptionUpdateObserver);
                });
    }

    @Override
    public void onEnabledStateChanged() {}

    @Override
    public void onFilterListsChanged() {}

    @Override
    public void onAllowedDomainsChanged() {}

    @Override
    public void onCustomFiltersChanged() {
        mHelper.notifyCalled();
    }

    public void addFilterList(final String filterListUrl) {
        mTestSubscriptionUrl = null;
        try {
            mTestSubscriptionUrl = new URL(filterListUrl);
            mObserver.setExpectedSubscriptionUrl(mTestSubscriptionUrl);
        } catch (MalformedURLException ignored) {
        }
        Assert.assertNotNull("Test subscription url", mTestSubscriptionUrl);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mAdblockFilteringConfiguration.addFilterList(mTestSubscriptionUrl);
                });
        try {
            mHelper.waitForCallback(0, 1, TEST_TIMEOUT_SEC, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            Assert.assertEquals(
                    "Test subscription was properly added", "Failed to add test subscription");
        }
    }

    public void addCustomFilter(final String filter) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mAdblockFilteringConfiguration.addCustomFilter(filter);
                });
        try {
            mHelper.waitForCallback(0, 1, TEST_TIMEOUT_SEC, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            Assert.assertEquals(
                    "Test custom filter was properly added", "Failed to add test custom filter");
        }
    }

    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (mAdblockFilteringConfiguration != null) {
                        mAdblockFilteringConfiguration.removeSubscriptionUpdateObserver(
                                mSubscriptionUpdateObserver);
                        mAdblockFilteringConfiguration.removeObserver(this);
                    }
                    if (mResourceClassificationNotifier != null) {
                        mResourceClassificationNotifier.removeResourceFilteringObserver(mObserver);
                    }
                });
    }

    // Note: Use either setOnAdMatchedLatch XOR setOnAdMatchedExpectations
    public void setOnAdMatchedLatch(final CountDownLatch countDownLatch) {
        Assert.assertTrue(
                mObserver.countDownLatch == null || mObserver.countDownLatch.getCount() == 0);
        mObserver.countDownLatch = countDownLatch;
    }

    // Note: Use either setOnAdMatchedLatch XOR setOnAdMatchedExpectations
    public CountDownLatch setOnAdMatchedExpectations(
            final Set<String> onBlocked, final Set<String> onAllowed) {
        Assert.assertTrue(
                mObserver.countDownLatch == null || mObserver.countDownLatch.getCount() == 0);
        mObserver.countDownLatch = new CountDownLatch(1);
        mObserver.expectedBlocked = onBlocked;
        mObserver.expectedAllowed = onAllowed;
        return mObserver.countDownLatch;
    }

    public boolean isBlocked(final String url) {
        return mObserver.isBlocked(url);
    }

    public boolean isPopupBlocked(final String url) {
        return mObserver.isPopupBlocked(url);
    }

    public int numBlockedByType(final ContentType type) {
        return mObserver.numBlockedByType(type);
    }

    public int numBlockedPopups() {
        return mObserver.numBlockedPopups();
    }

    public int numAllowedByType(final ContentType type) {
        return mObserver.numAllowedByType(type);
    }

    public int numAllowedPopups() {
        return mObserver.numAllowedPopups();
    }

    public boolean isAllowed(final String url) {
        return mObserver.isAllowed(url);
    }

    public boolean isPageAllowed(final String url) {
        return mObserver.isPageAllowed(url);
    }

    public boolean isPopupAllowed(final String url) {
        return mObserver.isPopupAllowed(url);
    }

    public void loadUrlWaitForContent(final String url) throws Exception {
        loadUrl(url);
        TestVerificationUtils.verifyCondition(
                this, "document.getElementsByClassName('testcase-waiting-content').length == 0");
    }

    public abstract void loadUrl(final String url) throws Exception;

    public abstract BrowserContextHandle getBrowserContext();

    public abstract WebContents getWebContents();

    public int numBlocked() {
        return mObserver.blockedInfos.size();
    }

    public int numAllowed() {
        return mObserver.allowedInfos.size();
    }
}
