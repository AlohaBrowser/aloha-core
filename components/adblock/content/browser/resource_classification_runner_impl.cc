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

#include "components/adblock/content/browser/resource_classification_runner_impl.h"

#include "base/functional/bind.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/content/browser/frame_opener_info.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/adblock/core/common/sitekey.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {

using ClassificationDecision =
    ResourceClassifier::ClassificationResult::Decision;

ResourceClassificationRunnerImpl::ResourceClassificationRunnerImpl(
    scoped_refptr<ResourceClassifier> resource_classifier,
    std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder,
    SitekeyStorage* sitekey_storage)
    : resource_classifier_(std::move(resource_classifier)),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)),
      sitekey_storage_(sitekey_storage),
      weak_ptr_factory_(this) {}

ResourceClassificationRunnerImpl::~ResourceClassificationRunnerImpl() = default;

void ResourceClassificationRunnerImpl::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void ResourceClassificationRunnerImpl::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void ResourceClassificationRunnerImpl::OnCheckPopupFilterMatchComplete(
    const GURL& popup_url,
    const std::vector<GURL>& frame_hierarchy,
    content::GlobalRenderFrameHostId render_frame_host_id,
    absl::optional<CheckFilterMatchCallback> callback,
    const ResourceClassifier::ClassificationResult& classification_result) {
  if (classification_result.decision != ClassificationDecision::Ignored) {
    FilterMatchResult result =
        classification_result.decision == ClassificationDecision::Allowed
            ? FilterMatchResult::kAllowRule
            : FilterMatchResult::kBlockRule;
    if (callback) {
      std::move(*callback).Run(result);
    }
    auto* frame_host = content::RenderFrameHost::FromID(render_frame_host_id);
    if (frame_host) {
      const auto& opener_url =
          frame_hierarchy.empty() ? GURL() : frame_hierarchy.front();
      if (result == FilterMatchResult::kBlockRule) {
        VLOG(1) << "[eyeo] Prevented loading of pop-up " << popup_url.spec()
                << ", opener: " << opener_url.spec();
      } else {
        VLOG(1) << "[eyeo] Pop-up allowed " << popup_url.spec()
                << ", opener: " << opener_url.spec();
      }
      for (auto& observer : observers_) {
        observer.OnPopupMatched(
            popup_url, result, opener_url, frame_host,
            classification_result.decisive_subscription,
            classification_result.decisive_configuration_name);
      }
    }
  } else if (callback) {
    std::move(*callback).Run(FilterMatchResult::kNoRule);
  }
}

FilterMatchResult ResourceClassificationRunnerImpl::ShouldBlockPopup(
    const SubscriptionService::Snapshot& subscription_collections,
    const GURL& popup_url,
    content::RenderFrameHost* frame_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] ShouldBlockPopup for " << popup_url.spec();
  DCHECK(frame_host);
  TRACE_EVENT1("eyeo", "ResourceClassificationRunnerImpl::ShouldBlockPopup",
               "popup_url", popup_url.spec());

  const auto frame_hierarchy = frame_hierarchy_builder_->BuildFrameHierarchy(
      RequestInitiator(frame_host));
  const auto sitekey = sitekey_storage_->FindSiteKeyForAnyUrl(frame_hierarchy);

  auto classification_result = resource_classifier_->ClassifyPopup(
      subscription_collections, popup_url, frame_hierarchy,
      sitekey ? sitekey->second : SiteKey());
  if (classification_result.decision == ClassificationDecision::Ignored) {
    return FilterMatchResult::kNoRule;
  }
  OnCheckPopupFilterMatchComplete(popup_url, frame_hierarchy,
                                  frame_host->GetGlobalId(), absl::nullopt,
                                  classification_result);
  return classification_result.decision == ClassificationDecision::Allowed
             ? FilterMatchResult::kAllowRule
             : FilterMatchResult::kBlockRule;
}

void ResourceClassificationRunnerImpl::CheckPopupFilterMatch(
    SubscriptionService::Snapshot subscription_collections,
    const GURL& popup_url,
    content::RenderFrameHost& render_frame_host,
    CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] CheckPopupFilterMatch for " << popup_url.spec();

  auto* wc = content::WebContents::FromRenderFrameHost(&render_frame_host);
  DCHECK(wc);
  auto* info = FrameOpenerInfo::FromWebContents(wc);
  DCHECK(info);
  auto* frame_host_opener = content::RenderFrameHost::FromID(info->GetOpener());
  if (!frame_host_opener) {
    // We are unable to check allowlisting
    VLOG(1) << "[eyeo] CheckPopupFilterMatch for " << popup_url.spec()
            << " no frame host for opener!";
    std::move(callback).Run(FilterMatchResult::kNoRule);
    return;
  }

  const auto frame_hierarchy = frame_hierarchy_builder_->BuildFrameHierarchy(
      RequestInitiator(frame_host_opener));
  auto sitekey = sitekey_storage_->FindSiteKeyForAnyUrl(frame_hierarchy);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(&ResourceClassifier::ClassifyPopup, resource_classifier_,
                     std::move(subscription_collections), popup_url,
                     frame_hierarchy, sitekey ? sitekey->second : SiteKey()),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::OnCheckPopupFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), popup_url, frame_hierarchy,
          render_frame_host.GetGlobalId(), std::move(callback)));
}

void ResourceClassificationRunnerImpl::CheckDocumentAllowlisted(
    SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const RequestInitiator& request_initiator) {
  // We pass main frame no matter what
  DVLOG(1) << "[eyeo] Main document. Passing it through: " << request_url;
  auto* render_frame_host = request_initiator.GetRenderFrameHost();
  DCHECK(render_frame_host);
  if (!render_frame_host) {
    // The frame has been destroyed, so we can't notify observers.
    return;
  }

  auto site_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(
      frame_hierarchy_builder_->BuildFrameHierarchy(request_initiator));
  SiteKey site_key;
  if (site_key_pair.has_value()) {
    site_key = site_key_pair->second;
    DVLOG(1) << "[eyeo] Found site key: " << site_key.value()
             << " for url: " << site_key_pair->first;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(&CheckDocumentAllowlistedInternal,
                     std::move(subscription_collections), request_url,
                     std::move(site_key)),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::ProcessDocumentAllowlistedResponse,
          weak_ptr_factory_.GetWeakPtr(), request_url,
          render_frame_host->GetGlobalId()));
}

void ResourceClassificationRunnerImpl::ProcessDocumentAllowlistedResponse(
    const GURL request_url,
    content::GlobalRenderFrameHostId render_frame_host_id,
    CheckResourceFilterMatchResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::RenderFrameHost* host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (result.status == FilterMatchResult::kAllowRule && host) {
    VLOG(1) << "[eyeo] Calling OnPageAllowed() for " << request_url;
    for (auto& observer : observers_) {
      observer.OnPageAllowed(request_url, host, result.subscription,
                             result.configuration_name);
    }
  }
}

void ResourceClassificationRunnerImpl::CheckRequestFilterMatch(
    SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    ContentType adblock_resource_type,
    const RequestInitiator& request_initiator,
    CheckFilterMatchCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] CheckRequestFilterMatchImpl for " << request_url.spec();

  const auto frame_hierarchy =
      frame_hierarchy_builder_->BuildFrameHierarchy(request_initiator);

  DVLOG(1) << "[eyeo] Got " << frame_hierarchy.size() << " frame_hierarchy for "
           << request_url.spec();

  auto site_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(frame_hierarchy);
  SiteKey site_key;
  if (site_key_pair.has_value()) {
    site_key = site_key_pair->second;
    DVLOG(1) << "[eyeo] Found site key: " << site_key.value()
             << " for url: " << site_key_pair->first;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(
          &ResourceClassificationRunnerImpl::CheckRequestFilterMatchInternal,
          resource_classifier_, std::move(subscription_collections),
          request_url, frame_hierarchy, adblock_resource_type,
          std::move(site_key)),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), request_url, frame_hierarchy,
          adblock_resource_type, request_initiator, std::move(callback)));
}

// static
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult
ResourceClassificationRunnerImpl::CheckRequestFilterMatchInternal(
    const scoped_refptr<ResourceClassifier>& resource_classifier,
    SubscriptionService::Snapshot subscription_collections,
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy,
    ContentType adblock_resource_type,
    const SiteKey sitekey) {
  TRACE_EVENT1("eyeo",
               "ResourceClassificationRunnerImpl::"
               "CheckRequestFilterMatchInternal",
               "url", request_url.spec());

  DVLOG(1) << "[eyeo] CheckRequestFilterMatchInternal start";

  auto classification_result = resource_classifier->ClassifyRequest(
      std::move(subscription_collections), request_url, frame_hierarchy,
      adblock_resource_type, sitekey);

  if (classification_result.decision == ClassificationDecision::Allowed) {
    VLOG(1) << "[eyeo] Resource allowed due to allowing filter " << request_url;
    return CheckResourceFilterMatchResult{
        FilterMatchResult::kAllowRule,
        classification_result.decisive_subscription,
        classification_result.decisive_configuration_name};
  }

  if (classification_result.decision == ClassificationDecision::Blocked) {
    VLOG(1) << "[eyeo] Resource blocked " << request_url;
    return CheckResourceFilterMatchResult{
        FilterMatchResult::kBlockRule,
        classification_result.decisive_subscription,
        classification_result.decisive_configuration_name};
  }

  return CheckResourceFilterMatchResult{FilterMatchResult::kNoRule, {}, {}};
}

void ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete(
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy,
    ContentType adblock_resource_type,
    const RequestInitiator& request_initiator,
    CheckFilterMatchCallback callback,
    const CheckResourceFilterMatchResult result) {
  // Notify |callback| as soon as we know whether we should block, as this
  // unblocks loading of network resources.
  std::move(callback).Run(result.status);
  // Only notify the UI if we explicitly blocked or allowed the resource, not
  // when there was NO_RULE.
  if (result.status == FilterMatchResult::kAllowRule ||
      result.status == FilterMatchResult::kBlockRule) {
    NotifyResourceMatched(request_url, result.status, frame_hierarchy,
                          adblock_resource_type, request_initiator,
                          result.subscription, result.configuration_name);
  }
}

void ResourceClassificationRunnerImpl::NotifyResourceMatched(
    const GURL& url,
    FilterMatchResult result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    const RequestInitiator& request_initiator,
    const GURL& subscription,
    const std::string& configuration_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  VLOG(1) << "[eyeo] NotifyResourceMatched() called for " << url;
  if (!request_initiator.IsFrame()) {
    // We don't have a frame to notify about, so we can't notify observers.
    return;
  }
  auto* render_frame_host = request_initiator.GetRenderFrameHost();
  if (!render_frame_host) {
    // The frame has been destroyed, so we can't notify observers.
    return;
  }

  for (auto& observer : observers_) {
    observer.OnRequestMatched(
        url, result, parent_frame_urls, static_cast<ContentType>(content_type),
        render_frame_host, subscription, configuration_name);
  }
}

// static
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult
ResourceClassificationRunnerImpl::CheckDocumentAllowlistedInternal(
    const SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const SiteKey sitekey) {
  CheckResourceFilterMatchResult result{FilterMatchResult::kNoRule, {}, {}};
  // It is required for all configurations to have an allowing Document filter
  // to consider a page allowlisted.
  for (const auto& collection : subscription_collections) {
    auto subscription_url = collection->FindBySpecialFilter(
        SpecialFilterType::Document, request_url, std::vector<GURL>(), sitekey);
    if (!subscription_url) {
      return {FilterMatchResult::kNoRule, {}, {}};
    } else {
      result = {FilterMatchResult::kAllowRule, subscription_url.value(),
                collection->GetFilteringConfigurationName()};
    }
  }
  return result;
}

void ResourceClassificationRunnerImpl::CheckResponseFilterMatch(
    SubscriptionService::Snapshot subscription_collections,
    const GURL& response_url,
    ContentType adblock_resource_type,
    const RequestInitiator& request_initiator,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    CheckFilterMatchCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] CheckResponseFilterMatch for " << response_url.spec();

  const auto frame_hierarchy =
      frame_hierarchy_builder_->BuildFrameHierarchy(request_initiator);
  // ResponseFilterMatch might take a while, let it run in the background.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(
          &ResourceClassificationRunnerImpl::CheckResponseFilterMatchInternal,
          resource_classifier_, std::move(subscription_collections),
          response_url, frame_hierarchy, adblock_resource_type,
          std::move(headers)),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), response_url, frame_hierarchy,
          adblock_resource_type, request_initiator, std::move(callback)));
}

// static
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult
ResourceClassificationRunnerImpl::CheckResponseFilterMatchInternal(
    const scoped_refptr<ResourceClassifier> resource_classifier,
    SubscriptionService::Snapshot subscription_collections,
    const GURL response_url,
    const std::vector<GURL> frame_hierarchy,
    ContentType adblock_resource_type,
    const scoped_refptr<net::HttpResponseHeaders> response_headers) {
  auto classification_result = resource_classifier->ClassifyResponse(
      std::move(subscription_collections), response_url, frame_hierarchy,
      adblock_resource_type, response_headers);

  if (classification_result.decision == ClassificationDecision::Allowed) {
    VLOG(1) << "[eyeo] Resource allowed due to allowing filter "
            << response_url;
    return CheckResourceFilterMatchResult{
        FilterMatchResult::kAllowRule,
        classification_result.decisive_subscription,
        classification_result.decisive_configuration_name};
  }

  if (classification_result.decision == ClassificationDecision::Blocked) {
    VLOG(1) << "[eyeo] Resource blocked " << response_url;
    return CheckResourceFilterMatchResult{
        FilterMatchResult::kBlockRule,
        classification_result.decisive_subscription,
        classification_result.decisive_configuration_name};
  }

  return CheckResourceFilterMatchResult{FilterMatchResult::kNoRule, {}, {}};
}

void ResourceClassificationRunnerImpl::CheckRewriteFilterMatch(
    SubscriptionService::Snapshot subscription_collections,
    const GURL& request_url,
    const RequestInitiator& request_initiator,
    base::OnceCallback<void(const absl::optional<GURL>&)> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << "[eyeo] CheckRewriteFilterMatch for " << request_url.spec();

  const auto frame_hierarchy =
      frame_hierarchy_builder_->BuildFrameHierarchy(request_initiator);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(&ResourceClassifier::CheckRewrite, resource_classifier_,
                     std::move(subscription_collections), request_url,
                     frame_hierarchy),
      std::move(callback));
}

}  // namespace adblock
