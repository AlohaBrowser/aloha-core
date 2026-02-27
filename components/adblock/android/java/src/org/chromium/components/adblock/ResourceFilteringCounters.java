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

import org.chromium.content_public.browser.GlobalRenderFrameHostId;

import java.util.List;

public class ResourceFilteringCounters {
    /** Immutable data-class containing an auxiliary information about resource event. */
    public static class ResourceInfo {
        // TODO: Make members private
        final String mRequestUrl;
        final List<String> mParentFrameUrls;
        final String mSubscriptionUrl;
        final String mConfigurationName;
        final ContentType mContentType;
        final GlobalRenderFrameHostId mMainFrameId;

        ResourceInfo(
                final String requestUrl,
                final List<String> parentFrameUrls,
                final String subscriptionUrl,
                final String configurationName,
                final int contentType,
                final GlobalRenderFrameHostId mainFrameId) {
            this.mRequestUrl = requestUrl;
            this.mParentFrameUrls = parentFrameUrls;
            this.mSubscriptionUrl = subscriptionUrl;
            this.mConfigurationName = configurationName;
            this.mContentType = ContentType.fromInt(contentType);
            this.mMainFrameId = mainFrameId;
        }

        /**
         * @return The request which was blocked or allowed.
         */
        public String getRequestUrl() {
            return mRequestUrl;
        }

        /**
         * @return The parent frames of the mRequestUrl.
         */
        public List<String> getParentFrameUrls() {
            return mParentFrameUrls;
        }

        /**
         * @return Subscription url for filter done blocking or allowing decision, empty string
         *     otherwise.
         */
        public String getSubscription() {
            return mSubscriptionUrl;
        }

        /**
         * @return Configuration name containing subscription with matched filter for blocking or
         *     allowing decision, empty string otherwise.
         */
        public String getConfigurationName() {
            return mConfigurationName;
        }

        /**
         * @return The resource content type. See enum ContentType in components/adblock/types.h
         */
        public ContentType getContentType() {
            return mContentType;
        }

        /**
         * @return The current tab id for the mRequestUrl. -1 means no tab id, likely tab closed
         *     before event arrived. Numbers start from 0.
         */
        public GlobalRenderFrameHostId getMainFrameId() {
            return mMainFrameId;
        }

        @Override
        public String toString() {
            return "mRequestUrl: "
                    + mRequestUrl
                    + ", mParentFrameUrls: "
                    + mParentFrameUrls.toString()
                    + ", mSubscriptionUrl:"
                    + mSubscriptionUrl
                    + ", mConfigurationName:"
                    + mConfigurationName
                    + ", mContentType: "
                    + mContentType.getValue()
                    + ", mMainFrameId: "
                    + mMainFrameId.frameRoutingId();
        }
    }

    /** Immutable data-class containing an auxiliary information about page element event. */
    public static class PageElementInfo {
        private final String mSelector;
        private final ElementHideAction mAction;

        final GlobalRenderFrameHostId mMainFrameId;

        PageElementInfo(
                final String selector,
                final int action,
                final GlobalRenderFrameHostId mainFrameId) {
            this.mSelector = selector;
            this.mAction = ElementHideAction.fromInt(action);
            this.mMainFrameId = mainFrameId;
        }

        /**
         * @return The selector which was targeted.
         */
        public String getSelector() {
            return mSelector;
        }

        /**
         * @return The action executed upon the selector.
         */
        public ElementHideAction getAction() {
            return mAction;
        }

        /**
         * @return The current tab id for the mRequestUrl. -1 means no tab id, likely tab closed
         *     before event arrived. Numbers start from 0.
         */
        public GlobalRenderFrameHostId getMainFrameId() {
            return mMainFrameId;
        }

        @Override
        public String toString() {
            return "mSelector: "
                    + mSelector
                    + ", mAction: "
                    + mAction.getValue()
                    + ", mMainFrameId: "
                    + mMainFrameId.frameRoutingId();
        }
    }
}
