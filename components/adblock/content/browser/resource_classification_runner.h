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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_H_

#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "services/network/public/mojom/network_param.mojom.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
}  // namespace content
namespace adblock {

/**
 * @brief Declares whether network requests should be blocked or allowed to
 * load by comparing their URLs and other metadata against filters defined
 * in active subscriptions.
 * Lives in the UI thread.
 */
class ResourceClassificationRunner : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Will only be called when |match_result| is BLOCK_RULE or ALLOW_RULE.
    virtual void OnRequestMatched(const GURL& url,
                                  FilterMatchResult match_result,
                                  const std::vector<GURL>& parent_frame_urls,
                                  ContentType content_type,
                                  content::RenderFrameHost* render_frame_host,
                                  const GURL& subscription,
                                  const std::string& configuration_name) = 0;
    virtual void OnPageAllowed(const GURL& url,
                               content::RenderFrameHost* render_frame_host,
                               const GURL& subscription,
                               const std::string& configuration_name) = 0;
    virtual void OnPopupMatched(const GURL& url,
                                FilterMatchResult match_result,
                                const GURL& opener_url,
                                content::RenderFrameHost* render_frame_host,
                                const GURL& subscription,
                                const std::string& configuration_name) = 0;
  };
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Performs a *synchronous* check, this can block the UI for a while!
  virtual FilterMatchResult ShouldBlockPopup(
      const SubscriptionService::Snapshot& subscription_collections,
      const GURL& popup_url,
      content::RenderFrameHost* render_frame_host) = 0;
  virtual void CheckPopupFilterMatch(
      SubscriptionService::Snapshot subscription_collections,
      const GURL& popup_url,
      content::RenderFrameHost& render_frame_host,
      CheckFilterMatchCallback callback) = 0;
  virtual void CheckRequestFilterMatch(
      SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      ContentType adblock_resource_type,
      const RequestInitiator& request_initiator,
      CheckFilterMatchCallback callback) = 0;
  // No callback, just notify observers
  virtual void CheckDocumentAllowlisted(
      SubscriptionService::Snapshot subscription_collection,
      const GURL& request_url,
      const RequestInitiator& request_initiator) = 0;
  virtual void CheckResponseFilterMatch(
      SubscriptionService::Snapshot subscription_collections,
      const GURL& response_url,
      ContentType adblock_resource_type,
      const RequestInitiator& request_initiator,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      CheckFilterMatchCallback callback) = 0;
  virtual void CheckRewriteFilterMatch(
      SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const RequestInitiator& request_initiator,
      base::OnceCallback<void(const absl::optional<GURL>&)> result) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_H_
