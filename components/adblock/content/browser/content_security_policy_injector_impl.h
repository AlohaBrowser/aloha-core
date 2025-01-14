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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_IMPL_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_IMPL_H_

#include <set>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/content/browser/content_security_policy_injector.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace adblock {

class ContentSecurityPolicyInjectorImpl final
    : public ContentSecurityPolicyInjector {
 public:
  ContentSecurityPolicyInjectorImpl(
      SubscriptionService* subscription_service,
      std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder);

  ~ContentSecurityPolicyInjectorImpl() final;

  void InsertContentSecurityPolicyHeadersIfApplicable(
      const GURL& request_url,
      const RequestInitiator& request_initiator,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      InsertContentSecurityPolicyHeadersCallback callback) final;

 private:
  void OnCspInjectionsSearchFinished(
      const GURL request_url,
      const scoped_refptr<net::HttpResponseHeaders> headers,
      InsertContentSecurityPolicyHeadersCallback callback,
      std::set<std::string> csp_injections);

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<SubscriptionService> subscription_service_;
  std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder_;
  base::WeakPtrFactory<ContentSecurityPolicyInjectorImpl> weak_ptr_factory{
      this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_IMPL_H_
