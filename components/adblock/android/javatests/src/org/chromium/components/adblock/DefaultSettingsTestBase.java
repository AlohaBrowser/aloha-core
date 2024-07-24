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
import org.junit.Test;

import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;

public abstract class DefaultSettingsTestBase {
    protected abstract BrowserContextHandle getBrowserContext();

    @Test
    @IntegrationTest
    @Feature({"adblock"})
    public void checkAdblockEnabledByDefault() {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    FilteringConfiguration adblockConfiguration =
                            FilteringConfiguration.createConfiguration(
                                    "adblock", getBrowserContext());

                    Assert.assertNotNull(adblockConfiguration);
                    Assert.assertTrue(adblockConfiguration.isEnabled());
                    Assert.assertNotEquals(adblockConfiguration.getFilterLists().size(), 0);
                });
    }

    @Test
    @IntegrationTest
    @Feature({"adblock"})
    public void checkAcceptableAdsEnabledByDefault() {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    FilteringConfiguration adblockConfiguration =
                            FilteringConfiguration.createConfiguration(
                                    "adblock", getBrowserContext());

                    Assert.assertNotNull(adblockConfiguration);
                    List<URL> filterLists = adblockConfiguration.getFilterLists();

                    try {
                        URL acceptableAdsUrl = new URL(adblockConfiguration.getAcceptableAdsUrl());
                        Assert.assertTrue(filterLists.contains(acceptableAdsUrl));
                    } catch (MalformedURLException e) {
                        Assert.fail();
                    }
                });
    }
}
