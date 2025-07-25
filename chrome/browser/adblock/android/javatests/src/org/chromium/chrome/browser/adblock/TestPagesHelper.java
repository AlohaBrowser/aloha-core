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

package org.chromium.chrome.browser.adblock;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.Tab.LoadUrlResult;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.adblock.TestPagesHelperBase;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.url.GURL;

import java.util.concurrent.TimeoutException;

public class TestPagesHelper extends TestPagesHelperBase {
    private ChromeTabbedActivityTestRule mActivityTestRule;

    public void setActivityTestRule(ChromeTabbedActivityTestRule activityTestRule) {
        mActivityTestRule = activityTestRule;
    }

    public void setUp(final ChromeTabbedActivityTestRule activityRule) {
        mActivityTestRule = activityRule;
        mActivityTestRule.startMainActivityOnBlankPage();
        mActivityTestRule.waitForActivityNativeInitializationComplete();
        super.setUp();
    }

    public CallbackHelper getTabsOpenedAndClosedWaiter() {
        final CallbackHelper callbackHelper = new CallbackHelper();
        final TabModelSelector tabModelSelector =
                mActivityTestRule.getActivity().getTabModelSelectorSupplier().get();
        Assert.assertNotNull(tabModelSelector);
        ThreadUtils.runOnUiThreadBlocking(
                () ->
                        tabModelSelector
                                .getCurrentModel()
                                .addObserver(
                                        new TabModelObserver() {
                                            @Override
                                            public void onFinishingTabClosure(Tab tab) {
                                                // For some reason TabModelObserver#tabRemoved() is
                                                // not called.
                                                // Let's wait a bit to make sure tab is indeed
                                                // closed.
                                                try {
                                                    Thread.sleep(100);
                                                } catch (InterruptedException ignored) {
                                                }
                                                callbackHelper.notifyCalled();
                                            }

                                            @Override
                                            public void didAddTab(
                                                    Tab tab,
                                                    int type,
                                                    int creationState,
                                                    boolean markedForSelection) {
                                                callbackHelper.notifyCalled();
                                            }
                                        }));
        return callbackHelper;
    }

    public CallbackHelper getTabsOpenedAndLoadedWaiter() {
        final CallbackHelper callbackHelper = new CallbackHelper();
        final TabModelSelector tabModelSelector =
                mActivityTestRule.getActivity().getTabModelSelectorSupplier().get();
        Assert.assertNotNull(tabModelSelector);
        ThreadUtils.runOnUiThreadBlocking(
                () ->
                        tabModelSelector
                                .getCurrentModel()
                                .addObserver(
                                        new TabModelObserver() {
                                            @Override
                                            public void didAddTab(
                                                    Tab tab,
                                                    int type,
                                                    int creationState,
                                                    boolean markedForSelection) {
                                                tab.addObserver(
                                                        new EmptyTabObserver() {
                                                            @Override
                                                            public void onPageLoadFinished(
                                                                    Tab tab, GURL url) {
                                                                callbackHelper.notifyCalled();
                                                            }
                                                        });
                                            }
                                        }));
        return callbackHelper;
    }

    public int getTabCount() {
        final TabModelSelector tabModelSelector =
                mActivityTestRule.getActivity().getTabModelSelectorSupplier().get();
        Assert.assertNotNull(tabModelSelector);
        return tabModelSelector.getTotalTabCount();
    }

    @Override
    public BrowserContextHandle getBrowserContext() {
        return ProfileManager.getLastUsedRegularProfile();
    }

    @Override
    public WebContents getWebContents() {
        return mActivityTestRule.getActivity().getCurrentWebContents();
    }

    @Override
    public void loadUrl(final String url) throws InterruptedException, TimeoutException {
        final LoadUrlResult result = mActivityTestRule.loadUrl(url, TEST_TIMEOUT_SEC);
        Assert.assertEquals(
                "Page did not load correctly. Load result enum: "
                        + String.valueOf(result.tabLoadStatus),
                Tab.TabLoadStatus.DEFAULT_PAGE_LOAD,
                result.tabLoadStatus);
    }
}
