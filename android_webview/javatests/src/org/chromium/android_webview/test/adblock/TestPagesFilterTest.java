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

package org.chromium.android_webview.test.adblock;

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.components.adblock.AdblockSwitches;
import org.chromium.components.adblock.TestPagesFilterTestBase;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.common.ContentSwitches;

@RunWith(AwJUnit4ClassRunner.class)
@CommandLineFlags.Add({AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING})
public class TestPagesFilterTest extends TestPagesFilterTestBase {
    @Rule public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private final TestPagesHelper mHelper = new TestPagesHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
        super.setUp(mHelper);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    private void testPopupCommon(final String linkParentId, final String popupPath)
            throws Exception {
        mHelper.loadUrl(TestPagesHelper.FILTER_TESTPAGES_TESTCASES_ROOT + "popup");
        // Popup action not yet triggered
        Assert.assertEquals(0, mHelper.numBlockedPopups());
        final String popupUrl = TestPagesHelper.TESTPAGES_RESOURCES_ROOT + "popup/" + popupPath;
        // Trigger popup action
        JavaScriptUtils.executeJavaScriptAndWaitForResult(
                mHelper.getWebContents(),
                "document.getElementById(\"" + linkParentId + "\").children[0].click();");
        Assert.assertEquals(1, mHelper.numBlockedPopups());
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    @CommandLineFlags.Add(ContentSwitches.DISABLE_POPUP_BLOCKING)
    public void testVerifyLinkBasedPopup() throws Exception {
        testPopupCommon("link-based-popup-area", "link.html");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    @CommandLineFlags.Add(ContentSwitches.DISABLE_POPUP_BLOCKING)
    public void testVerifyScriptBasedTabPopup() throws Exception {
        testPopupCommon("script-based-popup-tab-area", "script-tab.html");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    @CommandLineFlags.Add(ContentSwitches.DISABLE_POPUP_BLOCKING)
    public void testVerifyScriptBasedWindowPopup() throws Exception {
        testPopupCommon("script-based-popup-window-area", "script-window.html");
    }
}
