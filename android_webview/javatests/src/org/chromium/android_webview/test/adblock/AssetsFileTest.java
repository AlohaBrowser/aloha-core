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
import org.chromium.base.test.util.Feature;

import java.util.Arrays;
import java.util.HashSet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AwJUnit4ClassRunner.class)
public class AssetsFileTest {
    private static final String ASSET_FILE_URL = "file:///android_asset/adblock_frame_with_ad.html";
    private static final String AD_URL = "https://example.com/ad.png";
    @Rule public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();
    private final TestPagesHelper mHelper = new TestPagesHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testBlockingWebAdLoadedByAssetIframe() throws Exception {
        mHelper.addCustomFilter("ad.png");
        final CountDownLatch countDownLatch =
                mHelper.setOnAdMatchedExpectations(new HashSet<>(Arrays.asList(AD_URL)), null);
        mHelper.loadUrl(ASSET_FILE_URL);
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(0, mHelper.numAllowed());
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAllowingWebAdLoadedByAssetIframe() throws Exception {
        mHelper.addCustomFilter("ad.png");
        mHelper.addCustomFilter("@@" + ASSET_FILE_URL + "$document");
        final CountDownLatch countDownLatch =
                mHelper.setOnAdMatchedExpectations(null, new HashSet<>(Arrays.asList(AD_URL)));
        mHelper.loadUrl(ASSET_FILE_URL);
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(0, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numAllowed());
    }
}
