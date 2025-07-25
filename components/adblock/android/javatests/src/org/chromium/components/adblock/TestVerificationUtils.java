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

import org.junit.Assert;

import org.chromium.content_public.browser.test.util.JavaScriptUtils;

import java.util.Locale;
import java.util.concurrent.TimeoutException;

public class TestVerificationUtils {
    public enum IncludeSubframes {
        YES,
        NO,
    }

    private static final String STATUS_OK = "\"OK\"";

    private static final String MATCHES_HIDDEN_FUNCTION =
            "let matches = function(element) {"
                    + "  return window.getComputedStyle(element).display == \"none\";"
                    + "}";

    private static final String MATCHES_DISPLAYED_FUNCTION =
            "let matches = function(element) {"
                    + "  return window.getComputedStyle(element).display != \"none\";"
                    + "}";

    private static final String COUNT_ELEMENT_FUNCTION =
            "let countElements = function(selector, includeSubframes) {"
                    + "  let count = 0;"
                    + "  for (let element of document.querySelectorAll(selector)) {"
                    + "    if (matches(element))"
                    + "      ++count;"
                    + "  }"
                    + "  if (includeSubframes) {"
                    + "    for (let frame of document.querySelectorAll(\"iframe\")) {"
                    + "      for (let element of frame.contentWindow.document.body"
                    + ".querySelectorAll(selector)) {"
                    + "        if (matches(element))"
                    + "          ++count;"
                    + "      }"
                    + "    }"
                    + "  }"
                    + "  return count;"
                    + "}";

    // Poll every 100 ms until condition is met or 3 seconds timeout occurs
    // Internal timeout of JavaScriptUtils.runJavascriptWithAsyncResult() is 5 seconds
    // so our wait timeout needs to be shorter.
    private static final String WAIT_FUNCTION =
            "function waitWithTimeout() {"
                    + "  return new Promise(resolve => {"
                    + "    let repeat = 30;"
                    + "    const id = setInterval(() => {"
                    + "      --repeat;"
                    + "      if (%s) {" // predicate placeholder
                    + "        clearInterval(id);"
                    + "        resolve('OK');"
                    + "      } else if (repeat == 0) {"
                    + "        clearInterval(id);"
                    + "        resolve('Timeout');"
                    + "      }"
                    + "    }, 100);"
                    + "  });"
                    + "};"
                    + "waitWithTimeout().then((r) => { domAutomationController.send(r);});";

    private static void verifyMatchesCount(
            final TestPagesHelperBase helper,
            final int num,
            final String matchesFunction,
            final String selector,
            IncludeSubframes includeSubframes)
            throws TimeoutException {
        final String boolIncludeSubframes =
                includeSubframes == IncludeSubframes.YES ? "true" : "false";
        final String predicate =
                String.format(
                        Locale.getDefault(),
                        "countElements(\"%s\", %s) == %d",
                        selector,
                        boolIncludeSubframes,
                        num);
        final String waitFunction = String.format(WAIT_FUNCTION, predicate);
        final String js =
                String.format(
                        "(function () {%s\n%s\n%s\n}());",
                        matchesFunction, COUNT_ELEMENT_FUNCTION, waitFunction);
        final String result =
                JavaScriptUtils.runJavascriptWithAsyncResult(helper.getWebContents(), js);
        Assert.assertEquals(STATUS_OK, result);
    }

    public static void verifyHiddenCount(
            final TestPagesHelperBase helper, final int num, final String selector)
            throws TimeoutException {
        verifyHiddenCount(helper, num, selector, IncludeSubframes.YES);
    }

    public static void verifyHiddenCount(
            final TestPagesHelperBase helper,
            final int num,
            final String selector,
            final IncludeSubframes includeSubframes)
            throws TimeoutException {
        verifyMatchesCount(helper, num, MATCHES_HIDDEN_FUNCTION, selector, includeSubframes);
    }

    public static void verifyDisplayedCount(
            final TestPagesHelperBase helper, final int num, final String selector)
            throws TimeoutException {
        verifyDisplayedCount(helper, num, selector, IncludeSubframes.YES);
    }

    public static void verifyDisplayedCount(
            final TestPagesHelperBase helper,
            final int num,
            final String selector,
            final IncludeSubframes includeSubframes)
            throws TimeoutException {
        verifyMatchesCount(helper, num, MATCHES_DISPLAYED_FUNCTION, selector, includeSubframes);
    }

    public static void verifyCondition(final TestPagesHelperBase helper, final String predicate)
            throws TimeoutException {
        final String waitFunction = String.format(WAIT_FUNCTION, predicate);
        Assert.assertEquals(
                STATUS_OK,
                JavaScriptUtils.runJavascriptWithAsyncResult(
                        helper.getWebContents(), waitFunction));
    }

    public static void verifyGreenBackground(final TestPagesHelperBase helper, final String elemId)
            throws TimeoutException {
        verifyCondition(
                helper,
                "document.getElementById('"
                        + elemId
                        + "') && "
                        + "window.getComputedStyle(document.getElementById('"
                        + elemId
                        + "')).backgroundColor == 'rgb(13, 199, 75)'");
    }

    // For some cases it is better to rely on page script testing element
    // rather than invent a specific script to check condition. For example
    // checks for rewrite filters replaces content proper way.
    public static void verifySelfTestPass(final TestPagesHelperBase helper, final String elemId)
            throws TimeoutException {
        verifyCondition(
                helper,
                "document.getElementById('"
                        + elemId
                        + "') && "
                        + "document.getElementById('"
                        + elemId
                        + "').getAttribute('data-expectedresult') == 'pass'");
    }

    public static void expectResourceBlocked(final TestPagesHelperBase helper, final String elemId)
            throws TimeoutException {
        verifyCondition(
                helper,
                "document.getElementById('"
                        + elemId
                        + "') && "
                        + "window.getComputedStyle(document.getElementById('"
                        + elemId
                        + "')).display == 'none'");
    }

    public static void expectResourceShown(final TestPagesHelperBase helper, final String elemId)
            throws TimeoutException {
        verifyCondition(
                helper,
                "document.getElementById('"
                        + elemId
                        + "') && "
                        + "window.getComputedStyle(document.getElementById('"
                        + elemId
                        + "')).display == 'inline'");
    }

    public static void expectResourceStyleProperty(
            final TestPagesHelperBase helper,
            final String elemId,
            final String property,
            final String value)
            throws TimeoutException {
        verifyCondition(
                helper,
                "document.getElementById('"
                        + elemId
                        + "') && "
                        + "window.getComputedStyle(document.getElementById('"
                        + elemId
                        + "'))['"
                        + property
                        + "'] == '"
                        + value
                        + "'");
    }
}
