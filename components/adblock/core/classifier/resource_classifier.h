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

#ifndef COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_H_
#define COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace adblock {

// Classifies resources encountered on websites.
// May be called from multiple threads, thread-safe and immutable.
class ResourceClassifier
    : public base::RefCountedThreadSafe<ResourceClassifier> {
 public:
  struct ClassificationResult {
    enum class Decision {
      // The resource should be blocked as there's a blocking filter in at least
      // one of the Subscriptions.
      Blocked,
      // There is a blocking filter but at least one of the Subscriptions also
      // has an overriding allowing filter.
      Allowed,
      // There are no filters that apply to this resource.
      Ignored,
    } decision;
    // If decision is Blocked or Allowed, |decisive_subscription| has the URL of
    // the subscription that had the decisive filter.
    GURL decisive_subscription;
    // If decision is Blocked or Allowed, |decisive_configuration_name| has the
    // name of the FilteringConfiguration that contain matched filter.
    std::string decisive_configuration_name;
  };

  virtual ClassificationResult ClassifyRequest(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey) const = 0;

  virtual ClassificationResult ClassifyPopup(
      const SubscriptionService::Snapshot& subscription_collections,
      const GURL& popup_url,
      const std::vector<GURL>& opener_frame_hierarchy,
      const SiteKey& sitekey) const = 0;

  virtual ClassificationResult ClassifyResponse(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers)
      const = 0;

  virtual absl::optional<GURL> CheckRewrite(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy) const = 0;

 protected:
  friend class base::RefCountedThreadSafe<ResourceClassifier>;
  virtual ~ResourceClassifier();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_H_
