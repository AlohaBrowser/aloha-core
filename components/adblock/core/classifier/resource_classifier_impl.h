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

#ifndef COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_

#include <set>

#include "components/adblock/core/classifier/resource_classifier.h"

namespace adblock {

class ResourceClassifierImpl final : public ResourceClassifier {
 public:
  ResourceClassifierImpl() = default;

  ClassificationResult ClassifyRequest(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey) const final;

  ClassificationResult ClassifyPopup(
      const SubscriptionService::Snapshot& subscription_collections,
      const GURL& popup_url,
      const std::vector<GURL>& opener_frame_hierarchy,
      const SiteKey& sitekey) const final;

  ClassificationResult ClassifyResponse(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers)
      const final;

  absl::optional<GURL> CheckRewrite(
      const SubscriptionService::Snapshot subscription_collections,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy) const final;

 protected:
  friend class base::RefCountedThreadSafe<ResourceClassifierImpl>;
  ~ResourceClassifierImpl() override;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_
