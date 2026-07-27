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

import java.util.HashSet;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public abstract class TestPagesExceptionTestBase {
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyImageExceptions() throws Exception {
        final String subdomainImage =
                String.format(
                        "https://allowed.subdomain.%s/testfiles/image_exception/subdomain.png",
                        TestPagesHelperBase.TESTPAGES_DOMAIN);
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "image");
        Assert.assertEquals(2, mHelper.numAllowed());
        Assert.assertEquals(2, mHelper.numAllowedByType(ContentType.CONTENT_TYPE_IMAGE));
        Assert.assertTrue(mHelper.isAllowed(subdomainImage));
        Assert.assertTrue(
                mHelper.isAllowed(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "image_exception/image.png"));
        TestVerificationUtils.verifyDisplayedCount(mHelper, 2, "img");
        String numImages =
                JavaScriptUtils.executeJavaScriptAndWaitForResult(
                        mHelper.getWebContents(), "document.getElementsByTagName(\"img\").length;");
        Assert.assertEquals("2", numImages);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifySubdocumentException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "subdocument");
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(
                mHelper.isAllowed(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "subdocument_exception/subdocument.html"));
        TestVerificationUtils.verifyGreenBackground(mHelper, "exception-target");
        String numFrames =
                JavaScriptUtils.executeJavaScriptAndWaitForResult(
                        mHelper.getWebContents(), "window.frames.length;");
        Assert.assertEquals("1", numFrames);
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "iframe");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyScriptException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "script");
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(
                mHelper.isAllowed(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "script_exception/script.js"));
        TestVerificationUtils.verifyGreenBackground(mHelper, "script-target");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertTrue(
                mHelper.isBlocked(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "script_exception/image.png"));
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyStylesheetException() throws Exception {
        final String allowedUrl =
                TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT + "stylesheet_exception/stylesheet.cs";
        final CountDownLatch countDownLatch =
                mHelper.setOnAdMatchedExpectations(null, new HashSet<>(List.of(allowedUrl)));
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "stylesheet");
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(mHelper.isAllowed(allowedUrl));
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertTrue(
                mHelper.isBlocked(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "stylesheet_exception/image.png"));
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img");
        TestVerificationUtils.verifyGreenBackground(mHelper, "exception-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyXHRException() throws Exception {
        final String allowedUrl =
                TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT + "xmlhttprequest_exception/text.txt";
        final CountDownLatch countDownLatch =
                mHelper.setOnAdMatchedExpectations(null, new HashSet<>(List.of(allowedUrl)));
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "xmlhttprequest");
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(mHelper.isAllowed(allowedUrl));
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyGenericBlockException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "genericblock");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertTrue(
                mHelper.isBlocked(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT
                                + "genericblock/specific.png"));
        Assert.assertEquals(1, mHelper.numBlockedByType(ContentType.CONTENT_TYPE_IMAGE));
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "img[data-expectedresult='pass']");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "img[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyGenericHideException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "generichide");
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[data-expectedresult='pass']");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyDocumentException() throws Exception {
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "document");
        Assert.assertTrue(
                mHelper.isAllowed(
                        TestPagesHelperBase.TESTPAGES_RESOURCES_ROOT + "document/image.png"));
        Assert.assertTrue(
                mHelper.isPageAllowed(
                        TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "document"));
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyWebSocketException() throws Exception {
        final String wssUrl =
                String.format("wss://%s/exception_websocket", TestPagesHelperBase.TESTPAGES_DOMAIN);
        final CountDownLatch countDownLatch =
                mHelper.setOnAdMatchedExpectations(null, new HashSet<>(List.of(wssUrl)));
        mHelper.loadUrl(TestPagesHelperBase.EXCEPTION_TESTPAGES_TESTCASES_ROOT + "websocket");
        // Wait with 10 seconds max timeout
        countDownLatch.await(10, TimeUnit.SECONDS);
        Assert.assertEquals(1, mHelper.numAllowedByType(ContentType.CONTENT_TYPE_WEBSOCKET));
        Assert.assertTrue(mHelper.isAllowed(wssUrl));
    }
}
