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

#include "components/adblock/content/browser/content_security_policy_injector_impl.h"

#include <string_view>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {
namespace {

std::set<std::string> GetCspInjections(
    const SubscriptionService::Snapshot subscription_collections,
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy_chain) {
  TRACE_EVENT1("eyeo", "GetCspInjection", "url", request_url.spec());
  std::set<std::string> injections;
  for (const auto& collection : subscription_collections) {
    for (const auto& injection :
         collection->GetCspInjections(request_url, frame_hierarchy_chain)) {
      injections.emplace(injection);
    }
  }
  if (!injections.empty()) {
    VLOG(1) << "[eyeo] Will attempt to inject CSP header/s " << " for "
            << request_url;
    DVLOG(2) << "[eyeo] CSP headers for " << request_url << ":";
    for (const auto& filter : injections) {
      DVLOG(2) << "[eyeo] " << filter;
    }
  }
  return injections;
}

}  // namespace

ContentSecurityPolicyInjectorImpl::ContentSecurityPolicyInjectorImpl(
    SubscriptionService* subscription_service,
    std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder)
    : subscription_service_(subscription_service),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)) {}

ContentSecurityPolicyInjectorImpl::~ContentSecurityPolicyInjectorImpl() =
    default;

void ContentSecurityPolicyInjectorImpl::
    InsertContentSecurityPolicyHeadersIfApplicable(
        const GURL& request_url,
        const RequestInitiator& request_initiator,
        const scoped_refptr<net::HttpResponseHeaders>& headers,
        InsertContentSecurityPolicyHeadersCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // GetCspInjection might take a while, let it run in the background.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(
          &GetCspInjections, subscription_service_->GetCurrentSnapshot(),
          request_url,
          frame_hierarchy_builder_->BuildFrameHierarchy(request_initiator)),
      base::BindOnce(
          &ContentSecurityPolicyInjectorImpl::OnCspInjectionsSearchFinished,
          weak_ptr_factory.GetWeakPtr(), request_url, std::move(headers),
          std::move(callback)));
}

void ContentSecurityPolicyInjectorImpl::OnCspInjectionsSearchFinished(
    const GURL request_url,
    const scoped_refptr<net::HttpResponseHeaders> headers,
    InsertContentSecurityPolicyHeadersCallback callback,
    std::set<std::string> csp_injections) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!csp_injections.empty()) {
    for (const auto& c_i : csp_injections) {
      // Set the CSP header according to |csp_injection|.
      headers->AddHeader("Content-Security-Policy", c_i);
    }
    // We need to ensure parsed headers match raw headers. Send the updated
    // raw headers to NetworkService for parsing.
    content::GetNetworkService()->ParseHeaders(request_url, headers,
                                               std::move(callback));
  } else {
    // No headers are injected, no need to update parsed headers.
    std::move(callback).Run(nullptr);
  }
}

}  // namespace adblock
