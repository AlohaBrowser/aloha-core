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

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.MediumTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.messages.MessageDispatcherProvider;
import org.chromium.components.messages.MessageIdentifier;
import org.chromium.components.messages.MessageStateHandler;
import org.chromium.components.messages.MessagesTestHelper;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.List;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AdblockPopupMessageTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final String POPUP_HTML_PATH = "/chrome/test/data/android/popup_test.html";

    private String mPopupHtmlUrl;
    private EmbeddedTestServer mTestServer;

    private void isPopupMessageShown(boolean visible) {
        CriteriaHelper.pollUiThread(
                () -> {
                    final MessageDispatcher messageDispatcher =
                            MessageDispatcherProvider.from(
                                    mActivityTestRule.getActivity().getWindowAndroid());
                    assert messageDispatcher != null;
                    final List<MessageStateHandler> messages =
                            MessagesTestHelper.getEnqueuedMessages(
                                    messageDispatcher, MessageIdentifier.POPUP_BLOCKED);
                    assert (visible ? !messages.isEmpty() : messages.isEmpty());
                });
    }

    @Before
    public void setUp() throws Exception {
        // Create a new temporary instance to ensure the Class is loaded. Otherwise we will get a
        // ClassNotFoundException when trying to instantiate during startup.
        mActivityTestRule.startMainActivityOnBlankPage();

        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mPopupHtmlUrl = mTestServer.getURL(POPUP_HTML_PATH);
    }

    @After
    public void tearDown() {
        mTestServer.stopAndDestroyServer();
    }

    public int getTabCount() {
        final TabModelSelector tabModelSelector =
                mActivityTestRule.getActivity().getTabModelSelectorSupplier().get();
        Assert.assertNotNull(tabModelSelector);
        return tabModelSelector.getTotalTabCount();
    }

    @Test
    @MediumTest
    @Feature({"adblock"})
    public void popUpBlockedMessageVisibleWhenAbpEnabled() {
        isPopupMessageShown(false);
        mActivityTestRule.loadUrl(mPopupHtmlUrl);
        Assert.assertEquals(1, getTabCount());
        isPopupMessageShown(true);
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"disable-adblock"})
    @Feature({"adblock"})
    public void popUpBlockedMessageVisibleWhenAbpDisabled() {
        isPopupMessageShown(false);
        mActivityTestRule.loadUrl(mPopupHtmlUrl);
        Assert.assertEquals(1, getTabCount());
        isPopupMessageShown(true);
    }
}
