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

import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.components.adblock.AdblockSwitches;
import org.chromium.components.adblock.TestPagesElemhideTestBase;

@RunWith(AwJUnit4ClassRunner.class)
@CommandLineFlags.Add({AdblockSwitches.DISABLE_EYEO_REQUEST_THROTTLING})
public class TestPagesElemhideTest extends TestPagesElemhideTestBase {
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
}
