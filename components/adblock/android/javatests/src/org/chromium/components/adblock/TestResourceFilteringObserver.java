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

import java.net.URL;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;

public class TestResourceFilteringObserver
        implements ResourceClassificationNotifier.ResourceFilteringObserver {
    @Override
    public void onRequestAllowed(ResourceFilteringCounters.ResourceInfo info) {
        allowedInfos.add(info);
        Assert.assertEquals(info.getSubscription(), getExpectedSubscriptionUrl());
        CheckAndCountDownLatch(Decision.ALLOWED, info.getRequestUrl().split("\\?")[0]);
    }

    @Override
    public void onRequestBlocked(ResourceFilteringCounters.ResourceInfo info) {
        blockedInfos.add(info);
        Assert.assertEquals(info.getSubscription(), getExpectedSubscriptionUrl());
        CheckAndCountDownLatch(Decision.BLOCKED, info.getRequestUrl().split("\\?")[0]);
    }

    @Override
    public void onPageAllowed(ResourceFilteringCounters.ResourceInfo info) {
        allowedPageInfos.add(info);
        Assert.assertEquals(info.getSubscription(), getExpectedSubscriptionUrl());
    }

    @Override
    public void onPopupAllowed(ResourceFilteringCounters.ResourceInfo info) {
        allowedPopupsInfos.add(info);
        Assert.assertEquals(info.getSubscription(), getExpectedSubscriptionUrl());
    }

    @Override
    public void onPopupBlocked(ResourceFilteringCounters.ResourceInfo info) {
        blockedPopupsInfos.add(info);
        Assert.assertEquals(info.getSubscription(), getExpectedSubscriptionUrl());
    }

    public boolean isBlocked(String url) {
        for (ResourceFilteringCounters.ResourceInfo info : blockedInfos) {
            if (info.getRequestUrl().contains(url)) return true;
        }

        return false;
    }

    public boolean isPopupBlocked(String url) {
        for (ResourceFilteringCounters.ResourceInfo info : blockedPopupsInfos) {
            if (info.getRequestUrl().contains(url)) return true;
        }

        return false;
    }

    public int numBlockedByType(ContentType type) {
        int result = 0;
        for (ResourceFilteringCounters.ResourceInfo info : blockedInfos) {
            if (info.getContentType() == type) ++result;
        }
        return result;
    }

    public int numBlockedPopups() {
        return blockedPopupsInfos.size();
    }

    public boolean isAllowed(String url) {
        for (ResourceFilteringCounters.ResourceInfo info : allowedInfos) {
            if (info.getRequestUrl().contains(url)) return true;
        }

        return false;
    }

    public boolean isPageAllowed(String url) {
        for (ResourceFilteringCounters.ResourceInfo info : allowedPageInfos) {
            if (info.getRequestUrl().contains(url)) return true;
        }

        return false;
    }

    public boolean isPopupAllowed(String url) {
        for (ResourceFilteringCounters.ResourceInfo info : allowedPopupsInfos) {
            if (info.getRequestUrl().contains(url)) return true;
        }

        return false;
    }

    public int numAllowedByType(ContentType type) {
        int result = 0;
        for (ResourceFilteringCounters.ResourceInfo info : allowedInfos) {
            if (info.getContentType() == type) ++result;
        }
        return result;
    }

    public int numAllowedPopups() {
        return allowedPopupsInfos.size();
    }

    public void setExpectedSubscriptionUrl(URL url) {
        mTestSubscriptionUrl = url;
    }

    private String getExpectedSubscriptionUrl() {
        if (mTestSubscriptionUrl != null) return mTestSubscriptionUrl.toString();
        return "adblock:custom";
    }

    private enum Decision {
        ALLOWED,
        BLOCKED
    }

    // We either countDown() our latch for every filtered resource if there are no
    // specific expectations set (expectedAllowed == null && expectedBlocked == null),
    // or we countDown() only when all expectations have been met so when:
    // (expectedAllowed.isNullOrEmpty() && expectedBlocked.isNullOrEmpty()).
    private void CheckAndCountDownLatch(final Decision decision, final String url) {
        if (countDownLatch != null) {
            if (expectedBlocked == null && expectedAllowed == null) {
                countDownLatch.countDown();
            } else {
                if (decision == Decision.BLOCKED) {
                    if (expectedBlocked != null) {
                        expectedBlocked.remove(url);
                    }
                } else {
                    if (expectedAllowed != null) {
                        expectedAllowed.remove(url);
                    }
                }
                boolean expectationsMet =
                        (expectedAllowed == null || expectedAllowed.isEmpty())
                                && (expectedBlocked == null || expectedBlocked.isEmpty());
                if (expectationsMet) {
                    countDownLatch.countDown();
                }
            }
        }
    }

    private URL mTestSubscriptionUrl;
    public List<ResourceFilteringCounters.ResourceInfo> blockedInfos = new CopyOnWriteArrayList<>();
    public List<ResourceFilteringCounters.ResourceInfo> allowedInfos = new CopyOnWriteArrayList<>();
    public List<ResourceFilteringCounters.ResourceInfo> allowedPageInfos =
            new CopyOnWriteArrayList<>();
    public List<ResourceFilteringCounters.ResourceInfo> blockedPopupsInfos =
            new CopyOnWriteArrayList<>();
    public List<ResourceFilteringCounters.ResourceInfo> allowedPopupsInfos =
            new CopyOnWriteArrayList<>();
    CountDownLatch countDownLatch;
    Set<String> expectedAllowed;
    Set<String> expectedBlocked;
}
