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

public abstract class TestPagesRewriteTestBase {
    public static final String REWRITE_TEST_URL =
            TestPagesHelperBase.FILTER_TESTPAGES_TESTCASES_ROOT + "rewrite";
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteScript() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.js$rewrite=abp-resource:blank-js,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifyDisplayedCount(mHelper, 0, "div[id='script-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteStylesheet() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.css$rewrite=abp-resource:blank-css,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifyGreenBackground(mHelper, "stylesheet-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteSubdocument() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.html$rewrite=abp-resource:blank-html,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "subdocument-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteText() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.txt$rewrite=abp-resource:blank-text,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "text-status");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteGif() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/1x1.gif$rewrite=abp-resource:1x1-transparent-gif,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "1x1-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite2x2Png() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/2x2.png$rewrite=abp-resource:2x2-transparent-png,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "2x2-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite3x2Png() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/3x2.png$rewrite=abp-resource:3x2-transparent-png,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "3x2-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite32x32Png() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/32x32.png$rewrite=abp-resource:32x32-transparent-png,"
                            + "domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        TestVerificationUtils.verifySelfTestPass(mHelper, "32x32-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteAudio() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.mp3$rewrite=abp-resource:blank-mp3,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        final String js_verification_code =
                "document.getElementById('audio-area') && "
                        + "document.getElementById('audio-area').lastChild && "
                        + "document.getElementById('audio-area').lastChild.getAttribute"
                        + "('data-expectedresult') == \"pass\"";
        TestVerificationUtils.verifyCondition(mHelper, js_verification_code);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteVideo() throws Exception {
        mHelper.addCustomFilter(
                String.format(
                        "||%s/testfiles/rewrite/*.mp4$rewrite=abp-resource:blank-mp4,domain=%s",
                        TestPagesHelperBase.TESTPAGES_DOMAIN,
                        TestPagesHelperBase.TESTPAGES_DOMAIN));
        mHelper.loadUrl(REWRITE_TEST_URL);
        final String js_verification_code =
                "document.getElementById('video-area') && "
                        + "document.getElementById('video-area').lastChild && "
                        + "document.getElementById('video-area').lastChild.getAttribute"
                        + "('data-expectedresult') == \"pass\"";
        TestVerificationUtils.verifyCondition(mHelper, js_verification_code);
    }
}
