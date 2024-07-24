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

public abstract class TestPagesWildcardDomainTestBase {

    public static final String WILDCARD_DOMAIN_TESTPAGES_URL =
            "https://subdomain.abptestpages.org/en/filters/wildcard-domain";

    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainContainsWithWildcards() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='element-hiding-contains-with-wildcards-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainId() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[id='eh-id']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainAttributeSelector() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='element-hiding-attribute-selector-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainAbpProperties() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='element-hiding-emulation-basic-abp-properties-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainAbpHas() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='element-hiding-emulation-basic-abp-has-usage-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainChainedExtendedSelectors() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div[id='element-hiding-emulation-chained-extended-selectors-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainExtendedSelectorAndDomain() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div[id='element-hide-emulation-wildcard-in-extended-selector-and-wildcard-in-domain-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testWildcardDomainRegularExpression() throws Exception {
        mHelper.loadUrl(WILDCARD_DOMAIN_TESTPAGES_URL);
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div[id='element-hiding-emulation-regular-expression-in-abp-contains-fail-1']");
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div[id='element-hiding-emulation-regular-expression-in-abp-contains-fail-2']");
    }
}
