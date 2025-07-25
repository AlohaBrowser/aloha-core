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

package org.chromium.content_shell_apk.adblock;

import android.annotation.SuppressLint;

import org.chromium.base.ThreadUtils;
import org.chromium.components.adblock.TestPagesHelperBase;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content_shell.adblock.ShellBrowserContext;
import org.chromium.content_shell_apk.ContentShellActivity;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

public class TestPagesHelper extends TestPagesHelperBase {
    private ContentShellActivityTestRule mActivityTestRule;
    private ContentShellActivity mActivity;

    public void setUp(final ContentShellActivityTestRule activityRule) {
        mActivityTestRule = activityRule;
        mActivity = mActivityTestRule.launchContentShellWithUrlSync("about:blank");
        super.setUp();
    }

    @SuppressLint("VisibleForTests")
    @Override
    public WebContents getWebContents() {
        return mActivity.getActiveWebContents();
    }

    @Override
    public BrowserContextHandle getBrowserContext() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    return ShellBrowserContext.getDefault();
                });
    }

    @Override
    public void loadUrl(final String url) throws Exception {
        try {
            mActivityTestRule.loadUrl(
                    getWebContents().getNavigationController(),
                    new TestCallbackHelperContainer(getWebContents()),
                    new LoadUrlParams(url));
        } catch (Throwable t) {
        }
    }
}
