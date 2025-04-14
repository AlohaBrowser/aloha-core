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

#include "components/adblock/content/browser/adblock_webcontents_observer.h"

namespace adblock {

AdblockWebContentObserver::AdblockWebContentObserver(
    content::WebContents* web_contents,
    adblock::SubscriptionService* subscription_service,
    adblock::ElementHider* element_hider,
    adblock::SitekeyStorage* sitekey_storage,
    std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder,
    base::RepeatingCallback<void(content::RenderFrameHost*)> navigation_counter)
    : AdblockWebContentObserverBase(web_contents,
                                    subscription_service,
                                    element_hider,
                                    sitekey_storage,
                                    std::move(frame_hierarchy_builder),
                                    std::move(navigation_counter)),
      content::WebContentsUserData<adblock::AdblockWebContentObserver>(
          *web_contents) {}

AdblockWebContentObserver::~AdblockWebContentObserver() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdblockWebContentObserver);

}  // namespace adblock
