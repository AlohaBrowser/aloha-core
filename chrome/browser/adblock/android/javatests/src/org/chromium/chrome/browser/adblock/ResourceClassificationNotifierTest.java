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

import androidx.test.InstrumentationRegistry;
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
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockSwitches;
import org.chromium.components.adblock.FilteringConfiguration;
import org.chromium.components.adblock.ResourceClassificationNotifier;
import org.chromium.components.adblock.TestResourceFilteringObserver;
import org.chromium.content_public.common.ContentSwitches;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
    ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
    ContentSwitches.HOST_RESOLVER_RULES + "=MAP * 127.0.0.1",
    AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING
})
public class ResourceClassificationNotifierTest {
    @Rule public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private final CallbackHelper mHelper = new CallbackHelper();
    public FilteringConfiguration mConfiguration;
    public TestResourceFilteringObserver mResourceFilteringObserver =
            new TestResourceFilteringObserver();

    private EmbeddedTestServer mTestServer;
    private String mTestUrl;

    public void loadTestUrl() {
        mActivityTestRule.loadUrl(mTestUrl, 5);
    }

    @Before
    public void setUp() throws TimeoutException {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfiguration =
                            FilteringConfiguration.createConfiguration(
                                    "a", ProfileManager.getLastUsedRegularProfile());
                    ResourceClassificationNotifier.getInstance(
                                    ProfileManager.getLastUsedRegularProfile())
                            .addResourceFilteringObserver(mResourceFilteringObserver);
                    mHelper.notifyCalled();
                });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        mActivityTestRule.startMainActivityOnBlankPage();
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mTestUrl =
                mTestServer.getURLWithHostName(
                        "test.org", "/components/test/data/adblock/innermost_frame.html");
    }

    @After
    public void tearDown() {
        mTestServer.stopAndDestroyServer();
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void noNotificationWithoutBlocking() throws Exception {
        loadTestUrl();

        Assert.assertTrue(mResourceFilteringObserver.blockedInfos.isEmpty());
        Assert.assertTrue(mResourceFilteringObserver.allowedInfos.isEmpty());
        Assert.assertTrue(mResourceFilteringObserver.allowedPageInfos.isEmpty());
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void resourceBlockedByFilter() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfiguration.addCustomFilter("resource.png");
                    mHelper.notifyCalled();
                });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        // Observer was notified about the blocking
        Assert.assertTrue(mResourceFilteringObserver.isBlocked("resource.png"));
        Assert.assertTrue(mResourceFilteringObserver.allowedInfos.isEmpty());
        Assert.assertTrue(mResourceFilteringObserver.allowedPageInfos.isEmpty());
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void pageAllowed() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfiguration.addCustomFilter("resource.png");
                    mConfiguration.addAllowedDomain("test.org");
                    // In general "adblock" configuration does not interfere.
                    // But for onPageAllowed event all configurations must contain
                    // page allowing rule so let's add rule there.
                    AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                            .addAllowedDomain("test.org");
                    mHelper.notifyCalled();
                });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        // Observer was notified about the allowed resource
        Assert.assertTrue(mResourceFilteringObserver.blockedInfos.isEmpty());
        Assert.assertTrue(mResourceFilteringObserver.isAllowed("resource.png"));
        Assert.assertTrue(mResourceFilteringObserver.isPageAllowed("test.org"));
    }
}
