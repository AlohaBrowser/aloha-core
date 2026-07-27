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

import org.junit.Test;

import org.chromium.base.test.util.Feature;

public abstract class TestPagesCspTestBase {
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testCspAllSites() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "csp_all");
        TestVerificationUtils.verifyDisplayedCount(mHelper, 0, "img[id='all-sites-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testCspSpecificSite() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "csp_specific");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper,
                0,
                "img[id='specific-site-fail-1']",
                TestVerificationUtils.IncludeSubframes.NO);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testCspSpecificSiteFrameSrc() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "csp_specific");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div[id='sub-frame-error']", TestVerificationUtils.IncludeSubframes.NO);
        TestVerificationUtils.verifyDisplayedCount(
                mHelper,
                0,
                "div[id='sub-frame-error-details']",
                TestVerificationUtils.IncludeSubframes.NO);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testCspException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "csp");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div[id='unblock-javascript'] > img");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testCspGenericBlockException() throws Exception {
        mHelper.loadUrl(
                TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "csp_genericblock");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div[id='genericblock-javascript'] > img");
    }
}
