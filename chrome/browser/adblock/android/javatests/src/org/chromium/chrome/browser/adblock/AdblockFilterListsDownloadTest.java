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

package org.chromium.chrome.browser.adblock;

import android.view.View;
import android.widget.CheckBox;
import android.widget.ListView;

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.chrome.browser.adblock.settings.AdblockFilterListsAdapter;
import org.chromium.chrome.browser.adblock.settings.AdblockFilterListsFragment;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockController.Subscription;
import org.chromium.components.adblock.AdblockSwitches;

import java.net.URL;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * DPD-3100: the ad-blocking settings "Filter lists" screen lets a user add an additional language
 * filter list, and the selected list is downloaded. Selecting a row drives the real {@link
 * AdblockFilterListsAdapter} click path ({@code installSubscription}); the download is confirmed
 * against the live subscription service, so request throttling is disabled to keep the fetch
 * prompt.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
    ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
    AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING,
    AdblockSwitches.DISABLE_EYEO_SNIPPETS_LIBRARY_UPDATE
})
public class AdblockFilterListsDownloadTest {
    private static final int DOWNLOAD_TIMEOUT_SEC = 30;
    private static final long ROW_STATE_TIMEOUT_MS = 10000L;
    private static final long POLL_INTERVAL_MS = 100L;

    private final CallbackHelper mDownloadHelper = new CallbackHelper();
    private AdblockController mController;
    private URL mTargetUrl;

    private final AdblockController.SubscriptionUpdateObserver mDownloadObserver =
            new AdblockController.SubscriptionUpdateObserver() {
                @Override
                public void onSubscriptionDownloaded(final URL url) {
                    if (mTargetUrl != null && url.toString().contains(mTargetUrl.toString())) {
                        mDownloadHelper.notifyCalled();
                    }
                }
            };

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    @Rule
    public SettingsActivityTestRule<AdblockFilterListsFragment> mTestRule =
            new SettingsActivityTestRule<>(AdblockFilterListsFragment.class);

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @After
    public void tearDown() {
        if (mController == null) return;
        ThreadUtils.runOnUiThreadBlocking(
                () -> mController.removeSubscriptionUpdateObserver(mDownloadObserver));
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void testSelectingLanguageListInstallsAndDownloadsIt() throws Exception {
        mTestRule.startSettingsActivity();
        final AdblockFilterListsFragment fragment = mTestRule.getFragment();
        Assert.assertNotNull(fragment);

        final ListView listView = fragment.getListView();
        final AdblockFilterListsAdapter adapter = (AdblockFilterListsAdapter) listView.getAdapter();
        Assert.assertNotNull("Filter lists screen should present a list adapter", adapter);

        final int callCount = mDownloadHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mController =
                            AdblockController.getInstance(
                                    ProfileManager.getLastUsedRegularProfile());
                    mController.addSubscriptionUpdateObserver(mDownloadObserver);

                    final List<Subscription> installed = mController.getInstalledSubscriptions();
                    final int position = findSelectableLanguageList(adapter, installed);
                    Assert.assertTrue(
                            "Filter lists screen should offer an unselected language list",
                            position >= 0);
                    mTargetUrl = ((Subscription) adapter.getItem(position)).url();

                    // Make sure the target row is attached before it is clicked.
                    listView.setSelection(position);
                });

        final URL targetUrl = mTargetUrl;
        waitForTargetRow(listView, targetUrl, false);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    final View row = targetRow(listView, targetUrl);
                    Assert.assertNotNull("Target language list row should still be attached", row);
                    // The adapter's click handler branches on the checked state, so re-check it in
                    // the same message as the click; a stale read here would drive the uninstall.
                    final CheckBox checkBox = row.findViewById(R.id.checkbox);
                    Assert.assertFalse(
                            "Target language list row must be unchecked before the install click",
                            checkBox.isChecked());
                    row.performClick();
                });

        // The adapter calls notifyDataSetChanged() right after installSubscription, so the row
        // already on screen has to re-bind as checked. That checked state is still derived from
        // getInstalledSubscriptions(); what this adds over reading the list directly is
        // coverage of the re-bind itself, i.e. that the screen reflects the install.
        waitForTargetRow(listView, targetUrl, true);

        mDownloadHelper.waitForCallback(callCount, 1, DOWNLOAD_TIMEOUT_SEC, TimeUnit.SECONDS);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    final Subscription installed =
                            findByUrl(mController.getInstalledSubscriptions(), targetUrl);
                    Assert.assertNotNull(
                            "Selected language list should be in the installed set", installed);
                    // A configured but not-yet-downloaded url is reported as a placeholder with an
                    // empty version, so only a non-empty version proves the list really installed.
                    Assert.assertFalse(
                            "Selected language list should report an installed version",
                            installed.version().isEmpty());
                });
    }

    /** The laid-out row for {@code url}, or null when it is not currently attached. */
    private static View targetRow(final ListView listView, final URL url) {
        for (int i = 0; i < listView.getChildCount(); i++) {
            final View child = listView.getChildAt(i);
            if (url.toString().equals(String.valueOf(child.getTag()))) return child;
        }
        return null;
    }

    private static void waitForTargetRow(
            final ListView listView, final URL url, final boolean checked) {
        CriteriaHelper.pollUiThread(
                () -> {
                    final View row = targetRow(listView, url);
                    if (row == null) return false;
                    final CheckBox checkBox = row.findViewById(R.id.checkbox);
                    return checkBox.isChecked() == checked;
                },
                "Target language list row should be on screen and "
                        + (checked ? "checked" : "unchecked"),
                ROW_STATE_TIMEOUT_MS,
                POLL_INTERVAL_MS);
    }

    private static int findSelectableLanguageList(
            final AdblockFilterListsAdapter adapter, final List<Subscription> installed) {
        for (int i = 0; i < adapter.getCount(); i++) {
            final Subscription candidate = (Subscription) adapter.getItem(i);
            if (candidate.languages().length == 0) continue;
            if (!containsUrl(installed, candidate.url())) return i;
        }
        return -1;
    }

    private static Subscription findByUrl(final List<Subscription> subscriptions, final URL url) {
        for (final Subscription subscription : subscriptions) {
            if (subscription.url().equals(url)) return subscription;
        }
        return null;
    }

    private static boolean containsUrl(final List<Subscription> subscriptions, final URL url) {
        return findByUrl(subscriptions, url) != null;
    }
}
