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

import java.util.concurrent.TimeoutException;

public abstract class TestPagesInlineCssTestBase {
    public static final String INLINE_CSS_TESTPAGES_URL =
            TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "inline-css";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
    }

    private void verifyIsRed(final String elemId) throws TimeoutException {
        TestVerificationUtils.expectResourceStyleProperty(
                mHelper, elemId, "backgroundColor", "rgb(199, 13, 44)");
    }

    private void verifyIsGreen(final String elemId) throws TimeoutException {
        TestVerificationUtils.expectResourceStyleProperty(
                mHelper, elemId, "backgroundColor", "rgb(13, 199, 75)");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testInlineCssWithEhSelector() throws Exception {
        final String elemId = "inline-css-id";
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL);
        verifyIsRed(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL);
        verifyIsGreen(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testInlineCssWithEheSelector() throws Exception {
        final String elemId = "basic-abp-properties-usage-with-inline-css-fail-1";
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL + "-extended");
        verifyIsRed(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL + "-extended");
        verifyIsGreen(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testInlineCssWithEheSelectorInversion() throws Exception {
        final String elemId = "basic-not-abp-properties-usage-with-inline-css-fail";
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL + "-extended-inversion");
        verifyIsRed(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(INLINE_CSS_TESTPAGES_URL + "-extended-inversion");
        verifyIsGreen(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testInlineCssOnDOMMutation() throws Exception {
        final String elemId = "span-inline-css";
        mHelper.loadUrlWaitForContent(INLINE_CSS_TESTPAGES_URL + "-on-DOM-mutation");
        verifyIsRed(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrlWaitForContent(INLINE_CSS_TESTPAGES_URL + "-on-DOM-mutation");
        verifyIsGreen(elemId);
    }
}
