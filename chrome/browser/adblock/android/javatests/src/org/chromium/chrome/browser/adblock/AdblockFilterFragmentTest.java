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

import static org.mockito.Mockito.when;

import android.util.Pair;
import android.view.View;
import android.widget.CheckBox;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.adblock.settings.AdblockFilterListsFragment;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockController.Subscription;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AdblockFilterFragmentTest {
    @Mock private AdblockController mAdblockControllerMock;

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    @Rule
    public SettingsActivityTestRule<AdblockFilterListsFragment> mTestRule =
            new SettingsActivityTestRule<>(AdblockFilterListsFragment.class);

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mActivityTestRule.startMainActivityOnBlankPage();
        mTestRule.startSettingsActivity();
        ApplicationTestUtils.finishActivity(mTestRule.getActivity());
    }

    @Test
    @SmallTest
    @Feature({"adblock"})
    public void testLanguageFiltersNotEmpty() {
        mTestRule.startSettingsActivity();
        final AdblockFilterListsFragment fragment = mTestRule.getFragment();
        Assert.assertNotNull(fragment);
        Assert.assertNotEquals(0, fragment.getListView().getCount());
    }

    @Test
    @SmallTest
    @Feature({"adblock"})
    public void testAutoInstalledFilterListsCheckbox() throws MalformedURLException {
        AdblockController.setInstanceForTesting(mAdblockControllerMock);
        final String autoInstalledUrl = "https://auto_installed.com";
        final String manuallyInstalledUrl = "https://manually_installed.com";
        final String recommendedUrl = "https://some_recommendation.com";
        final Subscription autoInstalled =
                new Subscription(new URL(autoInstalledUrl), "auto", "", new String[] {}, true);
        final Subscription manuallyInstalled =
                new Subscription(
                        new URL(manuallyInstalledUrl), "manual", "", new String[] {}, false);
        final Subscription recommended =
                new Subscription(
                        new URL(recommendedUrl), "recommended", "", new String[] {}, false);
        when(mAdblockControllerMock.getRecommendedSubscriptions())
                .thenReturn(Arrays.asList(autoInstalled, manuallyInstalled, recommended));
        when(mAdblockControllerMock.getInstalledSubscriptions())
                .thenReturn(Arrays.asList(autoInstalled, manuallyInstalled));
        mTestRule.startSettingsActivity();
        final AdblockFilterListsFragment fragment = mTestRule.getFragment();
        Assert.assertNotNull(fragment);
        Assert.assertEquals(3, fragment.getListView().getCount());

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // Map: filter list URL => Pair(checkbox marked, checkbox enabled)
                    final Map<String, Pair<Boolean, Boolean>> expectations = new HashMap<>();
                    expectations.put(autoInstalledUrl, new Pair<>(true, false));
                    expectations.put(manuallyInstalledUrl, new Pair<>(true, true));
                    expectations.put(recommendedUrl, new Pair<>(false, true));
                    for (int i = 0; i < fragment.getListView().getCount(); i++) {
                        final View view = fragment.getListView().getChildAt(i);
                        final CheckBox checkBox = view.findViewById(R.id.checkbox);
                        Assert.assertEquals(
                                expectations.get(view.getTag().toString()).first,
                                checkBox.isChecked());
                        Assert.assertEquals(
                                expectations.get(view.getTag().toString()).second,
                                checkBox.isEnabled());
                    }
                });
    }
}
