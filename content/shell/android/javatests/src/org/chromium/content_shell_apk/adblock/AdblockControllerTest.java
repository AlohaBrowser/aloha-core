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

import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockControllerTestBase;
import org.chromium.content_shell.adblock.ShellBrowserContext;
import org.chromium.content_shell_apk.ContentShellActivity;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

@RunWith(BaseJUnit4ClassRunner.class)
public class AdblockControllerTest extends AdblockControllerTestBase {
    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    @Before
    public void setUp() {
        ContentShellActivity activity =
                mActivityTestRule.launchContentShellWithUrlSync("about:blank");
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mAdblockController =
                            AdblockController.getInstance(ShellBrowserContext.getDefault());
                });
    }
}
