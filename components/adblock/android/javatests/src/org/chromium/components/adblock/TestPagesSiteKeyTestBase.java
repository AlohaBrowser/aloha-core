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

package org.chromium.components.adblock;

import androidx.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Test;

import org.chromium.base.test.util.Feature;

public abstract class TestPagesSiteKeyTestBase {
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifySitekeyException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.SITEKEY_TESTPAGES_TESTCASES_ROOT);
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertTrue(
                mHelper.isBlocked(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT + "sitekey/outofframe.png"));
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(
                mHelper.isAllowed(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT + "sitekey/inframe.png"));
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div");
    }
}
