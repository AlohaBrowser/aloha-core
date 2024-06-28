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

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public abstract class AdblockControllerTestBase {
    private final CallbackHelper mHelper = new CallbackHelper();
    protected AdblockController mAdblockController;

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void addingAllowedDomains() throws TimeoutException {
        final List<String> allowedDomains = new ArrayList<>();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mAdblockController.addAllowedDomain("foobar.com");
                    mAdblockController.addAllowedDomain("domain.com/path/to/page.html");
                    mAdblockController.addAllowedDomain("domain.com/duplicate.html");
                    mAdblockController.addAllowedDomain("https://scheme.com/path.html");
                    mAdblockController.addAllowedDomain("gibberish");
                    allowedDomains.addAll(mAdblockController.getAllowedDomains());
                    mHelper.notifyCalled();
                });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        // We expect to see a sorted collection of domains (not URLs) without duplicates.
        ArrayList<String> expectedAllowedDomains = new ArrayList<String>();
        expectedAllowedDomains.add("domain.com");
        expectedAllowedDomains.add("foobar.com");
        expectedAllowedDomains.add("scheme.com");
        expectedAllowedDomains.add("www.gibberish.com");
        Assert.assertEquals(expectedAllowedDomains, allowedDomains);
    }
}
