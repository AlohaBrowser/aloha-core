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

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Test;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.net.test.EmbeddedTestServer;

import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public abstract class FilteringConfigurationTestBase {
    private final CallbackHelper mCallbackHelper = new CallbackHelper();
    private TestPagesHelperBase mHelper;
    public FilteringConfiguration mConfigurationA;
    public FilteringConfiguration mConfigurationB;
    private EmbeddedTestServer mTestServer;
    protected String mTestUrl;

    protected abstract void loadTestUrl() throws Exception;

    protected abstract BrowserContextHandle getBrowserContext();

    public void setUp(TestPagesHelperBase helper, String filePath) throws TimeoutException {
        mHelper = helper;
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mTestUrl = mTestServer.getURLWithHostName("test.org", filePath);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA =
                            FilteringConfiguration.createConfiguration("a", getBrowserContext());
                    mConfigurationB =
                            FilteringConfiguration.createConfiguration("b", getBrowserContext());
                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }

    public void tearDown() {
        mTestServer.stopAndDestroyServer();
    }

    private static class TestConfigurationChangeObserver
            implements FilteringConfiguration.ConfigurationChangeObserver {
        public volatile boolean mOnEnabledStateChangedCalled;
        public volatile boolean mOnFilterListsChanged;
        public volatile boolean mOnAllowedDomainsChanged;
        public volatile boolean mOnCustomFiltersChanged;

        public TestConfigurationChangeObserver() {
            mOnEnabledStateChangedCalled = false;
            mOnFilterListsChanged = false;
            mOnAllowedDomainsChanged = false;
            mOnCustomFiltersChanged = false;
        }

        @Override
        public void onEnabledStateChanged() {
            mOnEnabledStateChangedCalled = true;
        }

        @Override
        public void onFilterListsChanged() {
            mOnFilterListsChanged = true;
        }

        @Override
        public void onAllowedDomainsChanged() {
            mOnAllowedDomainsChanged = true;
        }

        @Override
        public void onCustomFiltersChanged() {
            mOnCustomFiltersChanged = true;
        }
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void addingAllowedDomains() throws Exception {
        final List<String> allowedDomainsA = new ArrayList<>();
        final List<String> allowedDomainsB = new ArrayList<>();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addAllowedDomain("foobar.com");
                    mConfigurationA.addAllowedDomain("domain.com/path/to/page.html");
                    mConfigurationA.addAllowedDomain("domain.com/duplicate.html");
                    allowedDomainsA.addAll(mConfigurationA.getAllowedDomains());

                    mConfigurationB.addAllowedDomain("https://scheme.com/path.html");
                    mConfigurationB.addAllowedDomain("https://second.com");
                    mConfigurationB.removeAllowedDomain("https://second.com");
                    mConfigurationB.addAllowedDomain("gibberish");
                    allowedDomainsB.addAll(mConfigurationB.getAllowedDomains());

                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        // We expect to see a sorted collection of domains (not URLs) without duplicates.
        ArrayList<String> expectedAllowedDomainsA = new ArrayList<String>();
        expectedAllowedDomainsA.add("domain.com");
        expectedAllowedDomainsA.add("foobar.com");
        Assert.assertEquals(expectedAllowedDomainsA, allowedDomainsA);

        // We expect not to see second.com because it was removed after being added.
        ArrayList<String> expectedAllowedDomainsB = new ArrayList<String>();
        expectedAllowedDomainsB.add("scheme.com");
        expectedAllowedDomainsB.add("www.gibberish.com");
        Assert.assertEquals(expectedAllowedDomainsB, allowedDomainsB);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void addingCustomFilters() throws Exception {
        final List<String> customFiltersA = new ArrayList<>();
        final List<String> customFiltersB = new ArrayList<>();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addCustomFilter("foobar.com");
                    mConfigurationA.addCustomFilter("foobar.com");
                    mConfigurationA.addCustomFilter("abc");
                    customFiltersA.addAll(mConfigurationA.getCustomFilters());

                    mConfigurationB.addCustomFilter("https://scheme.com/path.html");
                    mConfigurationB.addCustomFilter("https://second.com");
                    mConfigurationB.removeCustomFilter("https://second.com");
                    customFiltersB.addAll(mConfigurationB.getCustomFilters());

                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        // We expect to see a collection of custom filters without duplicates.
        // The order represents order of addition.
        ArrayList<String> expectedCustomFiltersA = new ArrayList<String>();
        expectedCustomFiltersA.add("foobar.com");
        expectedCustomFiltersA.add("abc");
        Assert.assertEquals(expectedCustomFiltersA, customFiltersA);

        // We expect not to see https://second.com because it was removed after being added.
        ArrayList<String> expectedCustomFiltersB = new ArrayList<String>();
        expectedCustomFiltersB.add("https://scheme.com/path.html");
        Assert.assertEquals(expectedCustomFiltersB, customFiltersB);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void addingFilterLists() throws Exception {
        final URL filterList1 = new URL("http://filters.com/list1.txt");
        final URL filterList2 = new URL("http://filters.com/list2.txt");
        final List<URL> filterListsA = new ArrayList<URL>();
        final List<URL> filterListsB = new ArrayList<URL>();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addFilterList(filterList1);
                    mConfigurationA.addFilterList(filterList2);
                    mConfigurationA.addFilterList(filterList1);
                    filterListsA.addAll(mConfigurationA.getFilterLists());

                    mConfigurationB.addFilterList(filterList1);
                    mConfigurationB.addFilterList(filterList2);
                    mConfigurationB.removeFilterList(filterList1);
                    filterListsB.addAll(mConfigurationB.getFilterLists());

                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        ArrayList<URL> expectedFilterListsA = new ArrayList<URL>();
        expectedFilterListsA.add(filterList1);
        expectedFilterListsA.add(filterList2);
        Assert.assertEquals(expectedFilterListsA, filterListsA);

        ArrayList<URL> expectedFilterListsB = new ArrayList<URL>();
        expectedFilterListsB.add(filterList2);
        Assert.assertEquals(expectedFilterListsB, filterListsB);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void aliasedConfigurations() throws Exception {
        final URL filterList1 = new URL("http://filters.com/list1.txt");
        final URL filterList2 = new URL("http://filters.com/list2.txt");
        final List<URL> filterListsA = new ArrayList<URL>();
        final List<URL> filterListsB = new ArrayList<URL>();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // Create a new FilteringConfiguration with a name of one that
                    // already exist.
                    FilteringConfiguration aliasedConfiguration =
                            FilteringConfiguration.createConfiguration("a", getBrowserContext());

                    // We add filter lists only to the original configuration instance.
                    mConfigurationA.addFilterList(filterList1);
                    mConfigurationA.addFilterList(filterList2);

                    // We check what filter lists are present in the original and in the aliased
                    // instance.
                    filterListsA.addAll(mConfigurationA.getFilterLists());
                    filterListsB.addAll(aliasedConfiguration.getFilterLists());

                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        ArrayList<URL> expectedFilterLists = new ArrayList<URL>();
        expectedFilterLists.add(filterList1);
        expectedFilterLists.add(filterList2);
        Assert.assertEquals(expectedFilterLists, filterListsA);
        Assert.assertEquals(expectedFilterLists, filterListsB);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void configurationChangeObserverNotified() throws Exception {
        final URL filterList1 = new URL("http://filters.com/list1.txt");
        final TestConfigurationChangeObserver observer = new TestConfigurationChangeObserver();
        Assert.assertFalse(observer.mOnEnabledStateChangedCalled);
        Assert.assertFalse(observer.mOnAllowedDomainsChanged);
        Assert.assertFalse(observer.mOnCustomFiltersChanged);
        Assert.assertFalse(observer.mOnFilterListsChanged);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // We'll create an aliased instance which will receive notifications triggered
                    // by
                    // changes made to the original instance.
                    FilteringConfiguration aliasedConfiguration =
                            FilteringConfiguration.createConfiguration("a", getBrowserContext());
                    aliasedConfiguration.addObserver(observer);

                    mConfigurationA.addFilterList(filterList1);
                    mConfigurationA.addAllowedDomain("test.com");
                    mConfigurationA.addCustomFilter("test.com");
                    mConfigurationA.setEnabled(false);

                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        Assert.assertTrue(observer.mOnEnabledStateChangedCalled);
        Assert.assertTrue(observer.mOnAllowedDomainsChanged);
        Assert.assertTrue(observer.mOnCustomFiltersChanged);
        Assert.assertTrue(observer.mOnFilterListsChanged);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void resourceBlockedByFilter() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addCustomFilter("resource.png");
                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        TestVerificationUtils.expectResourceBlocked(mHelper, "subresource");
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void resourceAllowedByFilter() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addCustomFilter("resource.png");
                    // Allowing filter for the mocked test.org domain that mTestServer hosts.
                    mConfigurationA.addCustomFilter("@@test.org$document");
                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        TestVerificationUtils.expectResourceShown(mHelper, "subresource");
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void resourceAllowedByAllowedDomain() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mConfigurationA.addCustomFilter("resource.png");
                    mConfigurationA.addAllowedDomain("test.org");
                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        TestVerificationUtils.expectResourceShown(mHelper, "subresource");
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void resourceBlockedBySecondConfiguration() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // ConfigurationA allows the resource.
                    // Allowing rules take precedence within a FilteringConfiguration.
                    mConfigurationA.addCustomFilter("resource.png");
                    mConfigurationA.addAllowedDomain("test.org");
                    // But ConfigurationB blocks the resource.
                    // Blocking takes precedence across FilteringConfigurations.
                    mConfigurationB.addCustomFilter("resource.png");
                    mCallbackHelper.notifyCalled();
                });
        mCallbackHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        loadTestUrl();
        TestVerificationUtils.expectResourceBlocked(mHelper, "subresource");
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void noBlockingWithoutFilters() throws Exception {
        loadTestUrl();
        TestVerificationUtils.expectResourceShown(mHelper, "subresource");
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void createAndRemoveConfiguration() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    // Check initial state.
                    FilteringConfiguration.removeConfiguration("adblock", getBrowserContext());
                    ArrayList<FilteringConfiguration> expectedConfigurations =
                            new ArrayList<FilteringConfiguration>();
                    expectedConfigurations.add(mConfigurationA);
                    expectedConfigurations.add(mConfigurationB);
                    Assert.assertEquals(
                            expectedConfigurations,
                            FilteringConfiguration.getConfigurations(getBrowserContext()));
                    // Add new configuration (twice) and check.
                    FilteringConfiguration configurationC =
                            FilteringConfiguration.createConfiguration("c", getBrowserContext());
                    FilteringConfiguration configurationC2 =
                            FilteringConfiguration.createConfiguration("c", getBrowserContext());
                    // Confirm this is the same reference.
                    Assert.assertEquals(configurationC, configurationC2);
                    expectedConfigurations.add(configurationC);
                    Assert.assertEquals(
                            expectedConfigurations,
                            FilteringConfiguration.getConfigurations(getBrowserContext()));
                    // Remove configuration "c" twice (2nd attempt has no effect) and check.
                    FilteringConfiguration.removeConfiguration("c", getBrowserContext());
                    FilteringConfiguration.removeConfiguration("c", getBrowserContext());
                    expectedConfigurations.remove(configurationC);
                    Assert.assertEquals(
                            expectedConfigurations,
                            FilteringConfiguration.getConfigurations(getBrowserContext()));
                });
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void cannotUseRemovedConfiguration() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    FilteringConfiguration configurationC =
                            FilteringConfiguration.createConfiguration("c", getBrowserContext());
                    configurationC.setEnabled(false);
                    Assert.assertFalse(configurationC.isEnabled());
                    FilteringConfiguration.removeConfiguration("c", getBrowserContext());
                    try {
                        configurationC.setEnabled(true);
                        Assert.fail();
                    } catch (final IllegalStateException e) {
                        // Expected
                    }
                    // Recreate.
                    configurationC =
                            FilteringConfiguration.createConfiguration("c", getBrowserContext());
                    Assert.assertTrue(configurationC.isEnabled());
                    configurationC.setEnabled(false);
                    Assert.assertFalse(configurationC.isEnabled());
                });
    }
}
