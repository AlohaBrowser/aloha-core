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

public abstract class TestPagesServiceWorkersTestBase {
    public static final String SERVICE_WORKERS_TEST_URL =
            TestPagesHelperBase.TESTPAGES_TESTCASES_ROOT + "service-worker/";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testBlockingWorker() throws Exception {
        mHelper.loadUrlWaitForContent(SERVICE_WORKERS_TEST_URL + "worker-itself");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#blocked-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAllowingWorker() throws Exception {
        mHelper.loadUrlWaitForContent(SERVICE_WORKERS_TEST_URL + "worker-itself");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#allowed-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testBlockingFromWorker() throws Exception {
        mHelper.loadUrlWaitForContent(SERVICE_WORKERS_TEST_URL + "worker-subresource");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#blocked-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAllowingFromWorker() throws Exception {
        mHelper.loadUrlWaitForContent(SERVICE_WORKERS_TEST_URL + "worker-subresource");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#allowed-target > div[data-expectedresult='pass']");
    }
}
