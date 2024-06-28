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

public abstract class TestPagesElemhideTestBase {
    public static final String ELEMENT_HIDING_TESTPAGES_URL =
            TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "element-hiding";
    public static final String ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL =
            TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "elemhide";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersIdSelector() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='eh-id']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersIdSelectorDoubleCurlyBraces() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='{{eh-id}}']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersClassSelector() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[class='eh-class']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersDescendantSelector() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[class='eh-descendant']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersSiblingSelector() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[class='eh-sibling']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector1() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='attribute-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector2() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='attribute-selector-2-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector3() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='attribute-selector-3-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersStartsWithSelector1() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='starts-with-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersStartsWithSelector2() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='starts-with-selector-2-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersEndsWithSelector1() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='ends-with-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersContains() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='contains-fail-1']");
    }

    // Exceptions:
    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersBasicException() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img[id='basic-usage-fail-1']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div[id='basic-usage-area'] > div[id='basic-usage-pass-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersIframeException() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img[id='iframe-fail-1']");
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[id='iframe-pass-1']");
    }
}
