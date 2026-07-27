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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public abstract class TestPagesRemoveTestBase {
    public static final String REMOVE_TESTPAGES_URL =
            TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "remove";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
    }

    private void verifyResourcePresent(final String elemId) throws TimeoutException {
        TestVerificationUtils.verifyCondition(
                mHelper, "document.getElementById('" + elemId + "') !=  null");
    }

    private void verifyResourceRemoved(final String elemId) throws TimeoutException {
        TestVerificationUtils.verifyCondition(
                mHelper, "document.getElementById('" + elemId + "') ==  null");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testOnPageElementMatched() throws Exception {
        final Set<String> expectedRemoved =
                new HashSet<>(
                        Arrays.asList(
                                "#remove-id",
                                "div[id='{{remove-id}}']",
                                ".remove-class",
                                ".testcase-area > .remove-descendant",
                                ".testcase-examplecontent + .remove-sibling",
                                "div[href=\"http://testcase-attribute-remove.com/\"]",
                                "div[href^=\"http://testcase-startswith-remove.com/\"]",
                                "div[height=\"40\"][width=\"40\"]",
                                "div[style=\"width: 42px;\"]",
                                "div[style^=\"width: 43px;\"]",
                                "div[style$=\"width: 44px;\"]",
                                "div[style*=\"width: 45px;\"]"));
        final CountDownLatch countDownLatch =
                mHelper.setOnPageElementMatchedExpectations(null, expectedRemoved, null);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(REMOVE_TESTPAGES_URL);
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(0, expectedRemoved.size());
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRemoveWithEhSelector() throws Exception {
        final String elemId = "remove-id";
        mHelper.loadUrl(REMOVE_TESTPAGES_URL);
        verifyResourcePresent(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(REMOVE_TESTPAGES_URL);
        verifyResourceRemoved(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRemoveWithEheSelector() throws Exception {
        final String elemId = "basic-abp-properties-usage-with-remove-fail-1";
        mHelper.loadUrl(REMOVE_TESTPAGES_URL + "-extended");
        verifyResourcePresent(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(REMOVE_TESTPAGES_URL + "-extended");
        verifyResourceRemoved(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRemoveWithEheSelectorInversion() throws Exception {
        final String elemId = "basic-not-abp-properties-usage-with-remove-fail";
        mHelper.loadUrl(REMOVE_TESTPAGES_URL + "-extended-inversion");
        verifyResourcePresent(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrl(REMOVE_TESTPAGES_URL + "-extended-inversion");
        verifyResourceRemoved(elemId);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRemoveOnDOMMutation() throws Exception {
        final String elemId = "input-remove";
        mHelper.loadUrlWaitForContent(REMOVE_TESTPAGES_URL + "-on-DOM-mutation");
        verifyResourcePresent(elemId);
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
        mHelper.loadUrlWaitForContent(REMOVE_TESTPAGES_URL + "-on-DOM-mutation");
        verifyResourceRemoved(elemId);
    }
}
