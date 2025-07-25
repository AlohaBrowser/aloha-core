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

import android.annotation.SuppressLint;

import org.junit.Assert;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwTestContainerView;
import org.chromium.android_webview.test.TestAwContentsClient;
import org.chromium.base.ThreadUtils;
import org.chromium.components.adblock.TestPagesHelperBase;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.WebContents;

public class TestPagesHelper extends TestPagesHelperBase {
    private AwActivityTestRule mActivityTestRule;
    private AwTestContainerView mTestContainerView;
    private TestAwContentsClient mContentsClient;

    public void setActivityTestRule(AwActivityTestRule activityTestRule) {
        mActivityTestRule = activityTestRule;
    }

    public void setUp(final AwActivityTestRule activityRule) {
        mActivityTestRule = activityRule;
        mContentsClient = new TestAwContentsClient();
        mTestContainerView = mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwActivityTestRule.enableJavaScriptOnUiThread(getAwContents());
        super.setUp();
    }

    public AwContents getAwContents() {
        return mTestContainerView.getAwContents();
    }

    @SuppressLint("VisibleForTests")
    @Override
    public WebContents getWebContents() {
        return getAwContents().getWebContents();
    }

    @Override
    public BrowserContextHandle getBrowserContext() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    return AwBrowserContext.getDefault();
                });
    }

    @Override
    public void loadUrl(final String url) throws Exception {
        final AwContents aw = getAwContents();
        mActivityTestRule.loadUrlSync(aw, mContentsClient.getOnPageFinishedHelper(), url);
        if (url.contains(TESTPAGES_DOMAIN)) {
            Assert.assertTrue(mActivityTestRule.getTitleOnUiThread(aw).contains("ABP Test Pages"));
        }
        mActivityTestRule.waitForVisualStateCallback(aw);
    }
}
