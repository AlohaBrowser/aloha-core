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

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.adblock.AdblockSwitches;
import org.chromium.components.adblock.TestPagesExceptionTestBase;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.common.ContentSwitches;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
    ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
    AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING
})
public class TestPagesExceptionTest extends TestPagesExceptionTestBase {
    @Rule public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private final TestPagesHelper mHelper = new TestPagesHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
        super.setUp(mHelper);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    @CommandLineFlags.Add(ContentSwitches.DISABLE_POPUP_BLOCKING)
    public void testVerifyPopupException() throws Exception {
        final String POPUP_TESTACE_URL =
                TestPagesHelper.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "popup";
        mHelper.loadUrl(POPUP_TESTACE_URL);
        Assert.assertEquals(1, mHelper.getTabCount());
        final CallbackHelper tabsLoadedWaiter = mHelper.getTabsOpenedAndLoadedWaiter();
        final WebContents webContents = mHelper.getWebContents();
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    try {
                        String numElements =
                                JavaScriptUtils.executeJavaScriptAndWaitForResult(
                                        webContents,
                                        "var elements ="
                                            + " document.getElementsByClassName(\"testcase-trigger\");for"
                                            + " (let i = 0; i < elements.length; ++i) {   "
                                            + " elements[i].click();}elements.length;");
                        Assert.assertEquals("3", numElements);
                    } catch (TimeoutException e) {
                        Assert.assertEquals("Popups were triggered", "Popups were NOT triggered");
                    }
                });
        // Wait for three tab loaded events
        tabsLoadedWaiter.waitForCallback(0, 3, TestPagesHelper.TEST_TIMEOUT_SEC, TimeUnit.SECONDS);
        Assert.assertEquals(4, mHelper.getTabCount());
        Assert.assertEquals(3, mHelper.numAllowedPopups());
        Assert.assertTrue(
                mHelper.isPopupAllowed(
                        TestPagesHelper.TESTPAGES_RESOURCES_ROOT + "popup_exception/link.html"));
        Assert.assertTrue(
                mHelper.isPopupAllowed(
                        TestPagesHelper.TESTPAGES_RESOURCES_ROOT
                                + "popup_exception/script-window.html"));
        Assert.assertTrue(
                mHelper.isPopupAllowed(
                        TestPagesHelper.TESTPAGES_RESOURCES_ROOT
                                + "popup_exception/script-tab.html"));
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }
}
