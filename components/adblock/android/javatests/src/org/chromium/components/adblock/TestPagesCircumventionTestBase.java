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
import org.chromium.content_public.browser.test.util.JavaScriptUtils;

public abstract class TestPagesCircumventionTestBase {
    private TestPagesHelperBase mHelper;

    public void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyCircumventionInlineStyleNotImportant() throws Exception {
        mHelper.loadUrl(
                TestPagesHelperBase.CIRCUMVENTION_TESTPAGES_TESTCASES_ROOT
                        + "inline-style-important");
        TestVerificationUtils.verifyHiddenCount(mHelper, 2, "div");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyCircumventionAnonymousFrameDocumentWrite() throws Exception {
        mHelper.loadUrl(
                TestPagesHelperBase.CIRCUMVENTION_TESTPAGES_TESTCASES_ROOT
                        + "anoniframe-documentwrite");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "span[data-expectedresult='fail']");
        String numHidden =
                JavaScriptUtils.executeJavaScriptAndWaitForResult(
                        mHelper.getWebContents(),
                        "var hiddenCount = 0;var elements ="
                                + " document.getElementById(\"write\").contentWindow"
                                + ".document.getElementsByTagName(\"span\");for (let i = 0; i <"
                                + " elements.length; ++i) {    if"
                                + " (window.getComputedStyle(elements[i]).display == \"none\") "
                                + "++hiddenCount;}hiddenCount;");
        Assert.assertEquals("1", numHidden);
    }
}
