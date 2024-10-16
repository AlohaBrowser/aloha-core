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

import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.components.adblock.ContentType;
import org.chromium.components.adblock.TestPagesHelperBase;
import org.chromium.components.adblock.TestVerificationUtils;

@RunWith(AwJUnit4ClassRunner.class)
public class AdblockAwSettingsTest {
    @Rule public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();
    private final TestPagesHelper mHelper = new TestPagesHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testSuppressUrlFilters() throws Exception {
        final AwSettings settings =
                mActivityTestRule.getAwSettingsOnUiThread(mHelper.getAwContents());
        Assert.assertTrue(settings.getContentFilteringEnabled());
        settings.setContentFilteringEnabled(false);
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "image");
        Assert.assertEquals(0, mHelper.numBlockedByType(ContentType.CONTENT_TYPE_IMAGE));
        settings.setContentFilteringEnabled(true);
        mHelper.loadUrl("about:blank");
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "blocking");
        Assert.assertEquals(6, mHelper.numBlockedByType(ContentType.CONTENT_TYPE_IMAGE));
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testSuppressCssFilters() throws Exception {
        final AwSettings settings =
                mActivityTestRule.getAwSettingsOnUiThread(mHelper.getAwContents());
        Assert.assertTrue(settings.getContentFilteringEnabled());
        settings.setContentFilteringEnabled(false);
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "element-hiding");
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[id='eh-id']");
        settings.setContentFilteringEnabled(true);
        mHelper.loadUrl("about:blank");
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "element-hiding");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='eh-id']");
    }
}
