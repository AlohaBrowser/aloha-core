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

import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;

public abstract class TestPagesSnippetsTestBase {
    private TestPagesHelperBase mHelper;

    protected void setUp(TestPagesHelperBase helper) {
        mHelper = helper;
        mHelper.addFilterList(TestPagesHelperBase.TESTPAGES_SUBSCRIPTION);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortCurrentInlineScriptBasic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-current-inline-script");
        // All "Abort" snippets cancel creation of the target div, so it won't be hidden - it will
        // not exist in DOM. Therefore we verify it's not displayed instead of verifying it's
        // hidden.
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortCurrentInlineScriptSearch() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-current-inline-script");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#search-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortCurrentInlineScriptRegex() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-current-inline-script");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#regex-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadBasic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadSubProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadFunctionProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#functionproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteBasic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteSubProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteFunctionProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "abort-on-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#functionproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadBasic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadSubProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadMultipleProperties() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-read");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#multipleproperties-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteBasic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteSubProperty() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteMultipleProperties() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "abort-on-iframe-property-write");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#multipleproperties-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsStatic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "p#hic-static-id");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsDynamic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "p#hic-dynamic-id");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsSearch() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div#search2-target > p.target");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#search1-target > p[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsRegex() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains");
        // "hic-regex-1" does not match regex, should remain displayed.
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "p#hic-regex-1");

        // "hic-regex-2" and "hic-regex-2" do match regex, should be hidden.
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "p#hic-regex-2");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "p#hic-regex-3");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsFrame() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "span#frame-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsAndMatchesStyleStatic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-contains-and-matches-style");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#static-usage-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#static-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsAndMatchesStyleDynamic() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-contains-and-matches-style");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#dynamic-target > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#dynamic-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsImage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-contains-image");
        TestVerificationUtils.verifyHiddenCount(mHelper, 2, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsVisibleTextBasicUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-contains-visible-text");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#parent-basic > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsVisibleTextContentUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-contains-visible-text");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#parent-content > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfHasAndMatchesStyleBasicUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-has-and-matches-style");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfHasAndMatchesStyleLegitElements() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-has-and-matches-style");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#comments-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfLabeledBy() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-labelled-by");
        TestVerificationUtils.verifyHiddenCount(mHelper, 1, "div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(mHelper, 1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathBasicStaticUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#basic-static-usage-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#basic-static-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3BasicStaticUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#basic-static-usage-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#basic-static-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathClassUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#class-usage-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#class-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3ClassUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#class-usage-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#class-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathIdStartsWith() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#hide-if-id-starts-with-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#hide-if-id-starts-with-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3IdStartsWith() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#hide-if-id-starts-with-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#hide-if-id-starts-with-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3Regex() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath3");
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div#hide-if-class-matches-regex-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3NormalizeAndJoinStrings() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath3");
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div#normalize-and-join-strings-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath3CastToNumber() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-matches-xpath3");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#cast-to-number-area > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#cast-to-number-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesComputedXPathClassChangesDynamically() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-matches-computed-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div#hide-when-class-changes-dynamically-based-on-a-string-found-in-another-element-area"
                    + " > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesComputedXPathClassMatchesRegex() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "hide-if-matches-computed-xpath");
        TestVerificationUtils.verifyHiddenCount(
                mHelper,
                1,
                "div#hide-when-class-matches-regex-group-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfShadowContainsBasicUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-shadow-contains");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#basic-target > p[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfShadowContainsRegexUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "hide-if-shadow-contains");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 2, "div#regex-target > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#regex-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testJsonPrune() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "json-prune?delay=100");
        // The object does not get hidden, it no longer exists in the DOM.
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#testcase-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testOverridePropertyRead() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "override-property-read");
        TestVerificationUtils.verifyHiddenCount(
                mHelper, 1, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testStripFetchQueryParameterBasicUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "strip-fetch-query-parameter");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#basic-target > div[data-expectedresult='fail']");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 1, "div#basic-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testStripFetchQueryParameterOtherUsage() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT
                        + "strip-fetch-query-parameter");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 2, "div#other-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testSimulateMouseEvent() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "simulate-mouse-event");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#simulate-mouse-event-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    @DisabledTest(message = "DPD-2832")
    public void testSkipVideo() throws Exception {
        mHelper.loadUrlWaitForContent(
                TestPagesHelperBase.SNIPPETS_TESTPAGES_TESTCASES_ROOT + "skip-video");
        TestVerificationUtils.verifyDisplayedCount(
                mHelper, 0, "div#skip-video-area > div[data-expectedresult='fail']");
    }
}
