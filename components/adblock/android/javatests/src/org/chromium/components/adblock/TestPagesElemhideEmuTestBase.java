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

public abstract class TestPagesElemhideEmuTestBase {
    public static final String ELEMENT_HIDING_EMULATION_TESTPAGES_URL =
            TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "element-hiding-emulation";
    public static final String ELEMENT_HIDING_EMULATION_EXCEPTIONS_TESTPAGES_URL =
            TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "element-hiding";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicAbpProperties() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='basic-abp-properties-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicAbpHas() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='basic-abp-has-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicHas() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='basic-has-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicAbpContains() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "span[id='basic-abp-contains-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicXpath() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "span[id='basic-xpath-usage-fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicHasText() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "span[id='basic-has-text-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersChainedExtendedSelectors() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='chained-extended-selectors-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersCaseInsensitiveExtendedSelectors() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='case-insensitive-extended-selectors-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersWildcardInExtendedSelector() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='wildcard-in-extended-selector-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersRegularExpressionInAbpProperties() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='regular-expression-in-abp-properties-fail-1']");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='regular-expression-in-abp-properties-fail-2']");
        // "Not a target" div is not hidden, does not match regular expression.
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div[class='testcase-examplecontent ehe-regex3']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersRegularExpressionInAbpContains() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='regular-expression-in-abp-contains-fail-1']");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='regular-expression-in-abp-contains-fail-2']");
    }

    // Exceptions:
    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersException() throws Exception {
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_EXCEPTIONS_TESTPAGES_URL);
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[id='exception-usage-pass-1']");
    }

    // DPD-2543
    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFiltersBasicAbpPropertiesWithUsupportedFilter() throws Exception {
        // Add broken ElemHideEmu filter
        mHelper.addCustomFilter("abptestpages.org#?#div:-unsupported-selector(width: 213px)");
        mHelper.loadUrl(ELEMENT_HIDING_EMULATION_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='basic-abp-properties-usage-fail-1']");
    }
}
