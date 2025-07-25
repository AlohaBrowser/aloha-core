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

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.components.adblock.AdblockSwitches;
import org.chromium.components.adblock.FilteringConfigurationTestBase;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.common.ContentSwitches;

import java.util.concurrent.TimeoutException;

@RunWith(AwJUnit4ClassRunner.class)
@CommandLineFlags.Add({
    ContentSwitches.HOST_RESOLVER_RULES + "=MAP * 127.0.0.1",
    AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING
})
public class FilteringConfigurationTest extends FilteringConfigurationTestBase {
    @Rule public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();
    private final TestPagesHelper mHelper = new TestPagesHelper();

    @Override
    protected BrowserContextHandle getBrowserContext() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    return AwBrowserContext.getDefault();
                });
    }

    @Override
    protected void loadTestUrl() throws Exception {
        mHelper.loadUrl(mTestUrl);
    }

    @Before
    public void setUp() throws TimeoutException {
        mHelper.setUp(mActivityTestRule);
        mHelper.setActivityTestRule(mActivityTestRule);
        super.setUp(mHelper, "/android_webview/test/data/adblock/innermost_frame.html");
    }

    @After
    @Override
    public void tearDown() {
        mHelper.tearDown();
        super.tearDown();
    }
}
