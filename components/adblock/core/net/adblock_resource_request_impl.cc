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

#include "components/adblock/core/net/adblock_resource_request_impl.h"

#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/app_info.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {
namespace {

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("adblock_resource_request", R"(
        semantics {
          sender: "AdblockResourceRequest"
          description:
            "A request to download ad-blocking related resource. "
          trigger:
            "Interval or when user selects a new filter list source"
          data:
            "Version (timestamp) of the filter list, if present. "
            "Application name (ex. Chromium) "
            "Application version (93.0.4572.0) "
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You enable or disable this feature via 'Ad blocking' setting."
          policy_exception_justification: "Not implemented."
        })");

GURL BuildUrlWithParams(const GURL& url, const std::string extra_query_params) {
  std::string query = base::StrCat(
      {"addonName=", "eyeo-chromium-sdk", "&addonVersion=", "2.0.0",
       "&application=", base::EscapeQueryParamValue(AppInfo::Get().name, true),
       "&applicationVersion=",
       base::EscapeQueryParamValue(AppInfo::Get().version, true), "&platform=",
       base::EscapeQueryParamValue(AppInfo::Get().client_os, true),
       "&platformVersion=", "1.0"});

  if (!extra_query_params.empty()) {
    query += "&";
    query += extra_query_params;
  }

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return url.ReplaceComponents(replacements);
}

}  // namespace

AdblockResourceRequestImpl::AdblockResourceRequestImpl(
    const net::BackoffEntry::Policy* backoff_policy,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AdblockRequestThrottle* request_throttle)
    : backoff_entry_(std::make_unique<net::BackoffEntry>(backoff_policy)),
      url_loader_factory_(url_loader_factory),
      request_throttle_(request_throttle),
      retry_timer_(std::make_unique<base::OneShotTimer>()),
      number_of_redirects_(0) {}

AdblockResourceRequestImpl::~AdblockResourceRequestImpl() = default;

void AdblockResourceRequestImpl::Start(GURL url,
                                       Method method,
                                       ResponseCallback response_callback,
                                       RetryPolicy retry_policy,
                                       const std::string extra_query_params) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!IsStarted()) << "Start() called twice";
  url_ = BuildUrlWithParams(url, extra_query_params);
  method_ = method;
  retry_policy_ = retry_policy;
  response_callback_ = std::move(response_callback);
  StartWhenRequestsAllowed();
}

void AdblockResourceRequestImpl::Redirect(
    GURL redirect_url,
    const std::string extra_query_params) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsStarted()) << "Redirect() called before Start()";
  DCHECK(url_ != redirect_url) << "Invalid redirect. Same URL";
  VLOG(1) << "[eyeo] Will redirect " << url_ << " to " << redirect_url;
  ++number_of_redirects_;
  url_ = BuildUrlWithParams(redirect_url, extra_query_params);
  StartWhenRequestsAllowed();
}

size_t AdblockResourceRequestImpl::GetNumberOfRedirects() const {
  return number_of_redirects_;
}

void AdblockResourceRequestImpl::StartWhenRequestsAllowed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  request_throttle_->RunWhenRequestsAllowed(base::BindOnce(
      &AdblockResourceRequestImpl::StartInternal, weak_factory_.GetWeakPtr()));
}

void AdblockResourceRequestImpl::StartInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN2("eyeo", "Downloading resource", this, "url",
                                    url_.spec(), "method", MethodToString());
  if (!url_loader_factory_) {
    // This happens in unit tests that have no network. The request will hang
    // indefinitely.
    return;
  }
  VLOG(1) << "[eyeo] Downloading " << url_;
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url_;
  request->method = MethodToString();
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  if (method_ == Method::GET) {
    loader_->DownloadToTempFile(
        url_loader_factory_.get(),
        base::BindOnce(&AdblockResourceRequestImpl::OnDownloadFinished,
                       // Unretained is safe because destruction of |this| will
                       // remove |loader_| and will abort the callback.
                       base::Unretained(this)));
  } else {
    loader_->DownloadHeadersOnly(
        url_loader_factory_.get(),
        base::BindOnce(&AdblockResourceRequestImpl::OnHeadersReceived,
                       base::Unretained(this)));
  }
}

void AdblockResourceRequestImpl::Retry() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsStarted()) << "Retry() called before Start()";
  if (!url_loader_factory_) {
    // This happens in unit tests that have no network.
    return;
  }
  backoff_entry_->InformOfRequest(false);
  VLOG(1) << "[eyeo] Will retry downloading " << url_ << " in "
          << backoff_entry_->GetTimeUntilRelease();
  retry_timer_->Start(
      FROM_HERE, backoff_entry_->GetTimeUntilRelease(),
      base::BindOnce(&AdblockResourceRequestImpl::StartWhenRequestsAllowed,
                     // Unretained is safe because destruction of |this| will
                     // remove |retry_timer_| and abort the callback.
                     base::Unretained(this)));
}

void AdblockResourceRequestImpl::OnDownloadFinished(
    base::FilePath downloaded_file) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0("eyeo", "Downloading resource", this);

  if (downloaded_file.empty() &&
      retry_policy_ == RetryPolicy::RetryUntilSucceeded) {
    Retry();
    return;
  }

  GURL::Replacements strip_query;
  strip_query.ClearQuery();
  GURL url = url_.ReplaceComponents(strip_query);
  response_callback_.Run(
      url, std::move(downloaded_file),
      loader_->ResponseInfo() ? loader_->ResponseInfo()->headers : nullptr);
  // response_callback_ may delete this, do not call any member variables now.
}

void AdblockResourceRequestImpl::OnHeadersReceived(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0("eyeo", "Downloading resource", this);

  if (!headers && retry_policy_ == RetryPolicy::RetryUntilSucceeded) {
    Retry();
    return;
  }

  response_callback_.Run(GURL(), base::FilePath(), headers);
  // response_callback_ may delete this, do not call any member variables now.
}

const char* AdblockResourceRequestImpl::MethodToString() const {
  return method_ == Method::GET ? net::HttpRequestHeaders::kGetMethod
                                : net::HttpRequestHeaders::kHeadMethod;
}

bool AdblockResourceRequestImpl::IsStarted() const {
  // url_ gets set in Start() and never reset
  return !url_.is_empty();
}

}  // namespace adblock
