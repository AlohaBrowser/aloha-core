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

#include "components/adblock/content/browser/adblock_url_loader_factory.h"

#include "base/barrier_closure.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "components/adblock/content/browser/content_security_policy_injector.h"
#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/content/browser/eyeo_document_info.h"
#include "components/adblock/content/browser/frame_opener_info.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/features.h"
#include "components/adblock/core/sitekey_storage.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/parsed_headers.mojom-forward.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace adblock {

namespace {

// Without USER_BLOCKING priority - in session restore case - posted
// callbacks can take over a minute to trigger. This is why such high
// priority is applied everywhere.
const base::TaskPriority kTaskResponsePriority =
    base::TaskPriority::USER_BLOCKING;

bool IsDocumentRequest(const network::ResourceRequest& request) {
  return !request.url.SchemeIsWSOrWSS() && !request.is_fetch_like_api &&
         request.destination == network::mojom::RequestDestination::kDocument;
}

ContentType ToAdblockResourceType(const network::ResourceRequest& request) {
  if (request.url.SchemeIsWSOrWSS()) {
    return ContentType::Websocket;
  }
  if (request.is_fetch_like_api) {
    // See https://crbug.com/611453
    return ContentType::Xmlhttprequest;
  }

  switch (request.destination) {
    case network::mojom::RequestDestination::kDocument:
    case network::mojom::RequestDestination::kIframe:
    case network::mojom::RequestDestination::kFrame:
    case network::mojom::RequestDestination::kFencedframe:
      return ContentType::Subdocument;
    case network::mojom::RequestDestination::kStyle:
    case network::mojom::RequestDestination::kXslt:
      return ContentType::Stylesheet;
    case network::mojom::RequestDestination::kScript:
    case network::mojom::RequestDestination::kWorker:
    case network::mojom::RequestDestination::kSharedWorker:
    case network::mojom::RequestDestination::kServiceWorker:
      return ContentType::Script;
    case network::mojom::RequestDestination::kImage:
      return ContentType::Image;
    case network::mojom::RequestDestination::kFont:
      return ContentType::Font;
    case network::mojom::RequestDestination::kObject:
    case network::mojom::RequestDestination::kEmbed:
      return ContentType::Object;
    case network::mojom::RequestDestination::kAudio:
    case network::mojom::RequestDestination::kTrack:
    case network::mojom::RequestDestination::kVideo:
      return ContentType::Media;
    case network::mojom::RequestDestination::kEmpty:
      // https://fetch.spec.whatwg.org/#concept-request-destination
      if (request.keepalive) {
        return ContentType::Ping;
      }
      return ContentType::Other;
    case network::mojom::RequestDestination::kWebBundle:
      return ContentType::WebBundle;
    case network::mojom::RequestDestination::kAudioWorklet:
    case network::mojom::RequestDestination::kDictionary:
    case network::mojom::RequestDestination::kJson:
    case network::mojom::RequestDestination::kManifest:
    case network::mojom::RequestDestination::kPaintWorklet:
    case network::mojom::RequestDestination::kReport:
    case network::mojom::RequestDestination::kSpeculationRules:
    case network::mojom::RequestDestination::kWebIdentity:
      return ContentType::Other;
  }
  NOTREACHED();
  return ContentType::Other;
}

}  // namespace

class AdblockURLLoaderFactory::InProgressRequest
    : public network::mojom::URLLoader,
      public network::mojom::URLLoaderClient {
 public:
  InProgressRequest(
      AdblockURLLoaderFactory* factory,
      mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);

  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const absl::optional<GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  void OnReceiveEarlyHints(
      ::network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(
      ::network::mojom::URLResponseHeadPtr head,
      ::mojo::ScopedDataPipeConsumerHandle body,
      absl::optional<mojo_base::BigBuffer> cached_metadata) override;
  void OnReceiveRedirect(const ::net::RedirectInfo& redirect_info,
                         ::network::mojom::URLResponseHeadPtr head) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnComplete(const ::network::URLLoaderCompletionStatus& status) override;

 private:
  using ProcessResponseHeadersCallback =
      base::OnceCallback<void(FilterMatchResult,
                              network::mojom::ParsedHeadersPtr)>;
  using CheckRewriteFilterMatchCallback =
      base::OnceCallback<void(const absl::optional<GURL>&)>;

  void OnBindingsClosed();
  void OnClientDisconnected();
  void Start(uint32_t options,
             network::ResourceRequest request,
             const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
             mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
             mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
             const absl::optional<GURL>& rewrite);
  void OnRequestFilterMatchResult(
      ::mojo::PendingReceiver<network::mojom::URLLoader> loader,
      uint32_t options,
      const network::ResourceRequest& request,
      ::mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      FilterMatchResult result);
  void OnRedirectFilterMatchResult(const net::RedirectInfo& redirect_info,
                                   network::mojom::URLResponseHeadPtr head,
                                   FilterMatchResult result);
  void OnProcessHeadersResult(
      ::network::mojom::URLResponseHeadPtr head,
      ::mojo::ScopedDataPipeConsumerHandle body,
      absl::optional<mojo_base::BigBuffer> cached_metadata,
      FilterMatchResult result,
      network::mojom::ParsedHeadersPtr parsed_headers);
  void OnRequestError(int error_code);
  void CheckFilterMatch(CheckFilterMatchCallback callback);
  void ProcessResponseHeaders(
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      ProcessResponseHeadersCallback callback);
  void CheckRewriteFilterMatch(CheckRewriteFilterMatchCallback callback);
  void OnRequestUrlClassified(CheckFilterMatchCallback callback,
                              FilterMatchResult result);
  void OnResponseHeadersClassified(
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      ProcessResponseHeadersCallback callback,
      FilterMatchResult result);
  void PostFilterMatchCallbackToUI(CheckFilterMatchCallback callback,
                                   FilterMatchResult result);
  void PostResponseHeadersCallbackToUI(
      ProcessResponseHeadersCallback callback,
      FilterMatchResult result,
      network::mojom::ParsedHeadersPtr parsed_headers);
  void PostRewriteCallbackToUI(
      base::OnceCallback<void(const absl::optional<GURL>&)> callback,
      const absl::optional<GURL>& url);
  void SetPreCommitUrlForFrame();
  bool ShouldIgnoreRequest() const;

  GURL request_url_;
  int request_id_;
  bool is_document_request_;
  ContentType adblock_resource_type_;
  const raw_ptr<AdblockURLLoaderFactory> factory_;
  // There are the mojo pipe endpoints between this proxy and the renderer.
  // Messages received by |client_receiver_| are forwarded to
  // |target_client_|.
  mojo::Remote<network::mojom::URLLoaderClient> target_client_;
  mojo::Receiver<network::mojom::URLLoader> loader_receiver_;
  // These are the mojo pipe endpoints between this proxy and the network
  // process. Messages received by |loader_receiver_| are forwarded to
  // |target_loader_|.
  mojo::Remote<network::mojom::URLLoader> target_loader_;
  mojo::Receiver<network::mojom::URLLoaderClient> client_receiver_{this};
  base::WeakPtrFactory<AdblockURLLoaderFactory::InProgressRequest>
      weak_factory_{this};
};

AdblockURLLoaderFactory::InProgressRequest::InProgressRequest(
    AdblockURLLoaderFactory* factory,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
    : request_url_(request.url),
      request_id_(request_id),
      is_document_request_(IsDocumentRequest(request)),
      adblock_resource_type_(ToAdblockResourceType(request)),
      factory_(factory),
      target_client_(std::move(client)),
      loader_receiver_(this, std::move(loader_receiver)) {
  if (!is_document_request_ && !ShouldIgnoreRequest()) {
    // Subresource requests may be rewritten (redirected to a local resource).
    // Check this before sending the request to the network process.
    CheckRewriteFilterMatch(base::BindOnce(
        &InProgressRequest::Start, weak_factory_.GetWeakPtr(), options, request,
        traffic_annotation, target_loader_.BindNewPipeAndPassReceiver(),
        client_receiver_.BindNewPipeAndPassRemote()));
  } else {
    // Main frame navigation requests are never rewritten, start immediately.
    Start(options, request, traffic_annotation,
          target_loader_.BindNewPipeAndPassReceiver(),
          client_receiver_.BindNewPipeAndPassRemote(), absl::nullopt);
  }

  // Calls |OnBindingsClosed| only after both disconnect handlers have been run.
  base::RepeatingClosure closure = base::BarrierClosure(
      2, base::BindOnce(&InProgressRequest::OnBindingsClosed,
                        weak_factory_.GetWeakPtr()));
  loader_receiver_.set_disconnect_handler(closure);
  client_receiver_.set_disconnect_handler(closure);
  target_client_.set_disconnect_handler(base::BindOnce(
      &InProgressRequest::OnClientDisconnected, weak_factory_.GetWeakPtr()));
}

void AdblockURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  if (target_loader_.is_bound()) {
    target_loader_->FollowRedirect(removed_headers, modified_headers,
                                   modified_cors_exempt_headers, new_url);
  }
}

void AdblockURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound()) {
    target_loader_->SetPriority(priority, intra_priority_value);
  }
}

void AdblockURLLoaderFactory::InProgressRequest::PauseReadingBodyFromNet() {
  if (target_loader_.is_bound()) {
    target_loader_->PauseReadingBodyFromNet();
  }
}

void AdblockURLLoaderFactory::InProgressRequest::ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound()) {
    target_loader_->ResumeReadingBodyFromNet();
  }
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {
  target_client_->OnReceiveEarlyHints(std::move(early_hints));
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head,
    mojo::ScopedDataPipeConsumerHandle body,
    absl::optional<mojo_base::BigBuffer> cached_metadata) {
  if (ShouldIgnoreRequest()) {
    VLOG(1) << "[eyeo] Unsupported scheme, allowing to load.";
    target_client_->OnReceiveResponse(std::move(head), std::move(body),
                                      std::move(cached_metadata));
    return;
  }

  SetPreCommitUrlForFrame();

  VLOG(1) << "[eyeo] Sending headers for processing: " << request_url_;
  client_receiver_.Pause();
  const scoped_refptr<net::HttpResponseHeaders>& headers = head->headers;
  ProcessResponseHeaders(
      headers, base::BindOnce(&InProgressRequest::OnProcessHeadersResult,
                              weak_factory_.GetWeakPtr(), std::move(head),
                              std::move(body), std::move(cached_metadata)));
}

void AdblockURLLoaderFactory::InProgressRequest::OnProcessHeadersResult(
    ::network::mojom::URLResponseHeadPtr head,
    ::mojo::ScopedDataPipeConsumerHandle body,
    absl::optional<mojo_base::BigBuffer> cached_metadata,
    FilterMatchResult result,
    network::mojom::ParsedHeadersPtr parsed_headers) {
  if (result == FilterMatchResult::kBlockRule) {
    OnRequestError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }
  if (parsed_headers) {
    // Headers were modified by ProcessResponseHeaders(). Raw headers must match
    // parsed headers.
    // |new_response_head| already contains the modified raw headers, update the
    // parsed headers.
    head->parsed_headers = std::move(parsed_headers);
  }

  target_client_->OnReceiveResponse(std::move(head), std::move(body),
                                    std::move(cached_metadata));
  client_receiver_.Resume();
}

void AdblockURLLoaderFactory::InProgressRequest::OnRequestError(
    int error_code) {
  target_client_->OnComplete(network::URLLoaderCompletionStatus(error_code));
  factory_->RemoveRequest(this);
}

void AdblockURLLoaderFactory::InProgressRequest::CheckFilterMatch(
    CheckFilterMatchCallback callback) {
  if (!factory_->CheckHostValid()) {
    PostFilterMatchCallbackToUI(std::move(callback),
                                FilterMatchResult::kNoRule);
    return;
  }

  auto subscription_service = factory_->config_.subscription_service;
  if (is_document_request_) {
    auto* host = content::RenderFrameHost::FromID(factory_->host_id_);
    auto* wc = content::WebContents::FromRenderFrameHost(host);
    DCHECK(wc);
    auto* info = FrameOpenerInfo::FromWebContents(wc);
    // We could use WebContents::HasLiveOriginalOpenerChain() to recognize a
    // popup here. The problem is that its companion method
    // WebContents::GetFirstWebContentsInLiveOriginalOpenerChain() returns not a
    // direct opener of a popup (which can be an embedded iframe) but the main
    // frame of the page which opened popup which is not enough for correct
    // allowlisting. Because of that we need to track an opener via
    // content::WebContentsUserData (see
    // AdblockWebContentObserver::DidOpenRequestedURL()), so for
    // consistency we everywhere check content::WebContentsUserData to find out
    // whether a request is a popup or to obtain its opener.
    bool is_popup = info && content::RenderFrameHost::FromID(info->GetOpener());
    if (is_popup) {
      factory_->config_.resource_classifier->CheckPopupFilterMatch(
          subscription_service->GetCurrentSnapshot(), request_url_,
          factory_->host_id_,
          base::BindOnce(
              &AdblockURLLoaderFactory::InProgressRequest::
                  OnRequestUrlClassified,
              weak_factory_.GetWeakPtr(),
              base::BindOnce(&AdblockURLLoaderFactory::InProgressRequest::
                                 PostFilterMatchCallbackToUI,
                             weak_factory_.GetWeakPtr(), std::move(callback))));
    } else {
      factory_->config_.resource_classifier->CheckDocumentAllowlisted(
          subscription_service->GetCurrentSnapshot(), request_url_,
          factory_->host_id_);
      PostFilterMatchCallbackToUI(std::move(callback),
                                  FilterMatchResult::kNoRule);
    }
  } else {
    factory_->config_.resource_classifier->CheckRequestFilterMatch(
        subscription_service->GetCurrentSnapshot(), request_url_,
        adblock_resource_type_, factory_->host_id_,
        base::BindOnce(
            &AdblockURLLoaderFactory::InProgressRequest::OnRequestUrlClassified,
            weak_factory_.GetWeakPtr(),
            base::BindOnce(&AdblockURLLoaderFactory::InProgressRequest::
                               PostFilterMatchCallbackToUI,
                           weak_factory_.GetWeakPtr(), std::move(callback))));
  }
}

void AdblockURLLoaderFactory::InProgressRequest::ProcessResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    ProcessResponseHeadersCallback callback) {
  if (!factory_->CheckHostValid()) {
    PostResponseHeadersCallbackToUI(std::move(callback),
                                    FilterMatchResult::kNoRule, nullptr);
    return;
  }

  auto subscription_service = factory_->config_.subscription_service;
  factory_->config_.resource_classifier->CheckResponseFilterMatch(
      subscription_service->GetCurrentSnapshot(), request_url_,
      adblock_resource_type_, factory_->host_id_, headers,
      base::BindOnce(
          &AdblockURLLoaderFactory::InProgressRequest::
              OnResponseHeadersClassified,
          weak_factory_.GetWeakPtr(), headers,
          base::BindOnce(&AdblockURLLoaderFactory::InProgressRequest::
                             PostResponseHeadersCallbackToUI,
                         weak_factory_.GetWeakPtr(), std::move(callback))));
}

void AdblockURLLoaderFactory::InProgressRequest::CheckRewriteFilterMatch(
    CheckRewriteFilterMatchCallback callback) {
  if (!factory_->CheckHostValid()) {
    PostRewriteCallbackToUI(std::move(callback), absl::optional<GURL>{});
    return;
  }

  auto subscription_service = factory_->config_.subscription_service;
  factory_->config_.resource_classifier->CheckRewriteFilterMatch(
      subscription_service->GetCurrentSnapshot(), request_url_,
      factory_->host_id_,
      base::BindOnce(
          &AdblockURLLoaderFactory::InProgressRequest::PostRewriteCallbackToUI,
          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdblockURLLoaderFactory::InProgressRequest::OnRequestUrlClassified(
    CheckFilterMatchCallback callback,
    FilterMatchResult result) {
  if (result == FilterMatchResult::kBlockRule) {
    if (factory_->CheckHostValid()) {
      if (is_document_request_) {
        // This path means we classified popup
        auto* wc = content::WebContents::FromRenderFrameHost(
            content::RenderFrameHost::FromID(factory_->host_id_));
        DCHECK(wc);
        wc->ClosePage();
      } else {
        ElementHider* element_hider = factory_->config_.element_hider;
        if (element_hider->IsElementTypeHideable(adblock_resource_type_)) {
          element_hider->HideBlockedElement(
              request_url_,
              content::RenderFrameHost::FromID(factory_->host_id_));
        }
      }
    }
  }
  PostFilterMatchCallbackToUI(std::move(callback), result);
}

void AdblockURLLoaderFactory::InProgressRequest::OnResponseHeadersClassified(
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    ProcessResponseHeadersCallback callback,
    FilterMatchResult result) {
  if (!factory_->CheckHostValid() || result == FilterMatchResult::kDisabled) {
    PostResponseHeadersCallbackToUI(std::move(callback), result, nullptr);
    return;
  }

  if (result == FilterMatchResult::kBlockRule) {
    ElementHider* element_hider = factory_->config_.element_hider;
    if (element_hider->IsElementTypeHideable(adblock_resource_type_)) {
      element_hider->HideBlockedElement(
          request_url_, content::RenderFrameHost::FromID(factory_->host_id_));
    }
    PostResponseHeadersCallbackToUI(std::move(callback), result, nullptr);
    return;
  }

  if (adblock_resource_type_ == ContentType::Subdocument) {
    factory_->config_.sitekey_storage->ProcessResponseHeaders(
        request_url_, headers, factory_->user_agent_string_);

    factory_->config_.csp_injector
        ->InsertContentSecurityPolicyHeadersIfApplicable(
            request_url_, factory_->host_id_, headers,
            base::BindOnce(std::move(callback), result));
  } else {
    PostResponseHeadersCallbackToUI(std::move(callback), result, nullptr);
  }
}

void AdblockURLLoaderFactory::InProgressRequest::PostFilterMatchCallbackToUI(
    CheckFilterMatchCallback callback,
    FilterMatchResult result) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), result));
}

void AdblockURLLoaderFactory::InProgressRequest::
    PostResponseHeadersCallbackToUI(
        ProcessResponseHeadersCallback callback,
        FilterMatchResult result,
        network::mojom::ParsedHeadersPtr parsed_headers) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), result,
                                           std::move(parsed_headers)));
}

void AdblockURLLoaderFactory::InProgressRequest::PostRewriteCallbackToUI(
    base::OnceCallback<void(const absl::optional<GURL>&)> callback,
    const absl::optional<GURL>& url) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), url));
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  VLOG(1)
      << "[eyeo] AdblockURLLoaderFactory::InProgressRequest::OnReceiveRedirect "
         "from "
      << request_url_ << " to " << redirect_info.new_url;
  request_url_ = redirect_info.new_url;
  if (ShouldIgnoreRequest()) {
    VLOG(1) << "[eyeo] Unsupported scheme, allowing to load.";
    target_client_->OnReceiveRedirect(redirect_info, std::move(head));
    return;
  }
  CheckFilterMatch(base::BindOnce(
      &InProgressRequest::OnRedirectFilterMatchResult,
      weak_factory_.GetWeakPtr(), redirect_info, std::move(head)));
}

void AdblockURLLoaderFactory::InProgressRequest::OnRedirectFilterMatchResult(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head,
    FilterMatchResult result) {
  if (!factory_->target_factory_.is_bound()) {
    DLOG(WARNING) << "[eyeo] "
                     "AdblockURLLoaderFactory::InProgressRequest::"
                     "OnRedirectFilterMatchResult: target_factory_ not bound";
    return;
  }
  if (result == FilterMatchResult::kBlockRule) {
    if (is_document_request_ && factory_->CheckHostValid()) {
      // This path means we classified popup
      auto* wc = content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(factory_->host_id_));
      DCHECK(wc);
      wc->ClosePage();
    }
    OnRequestError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }
  target_client_->OnReceiveRedirect(redirect_info, std::move(head));
}

void AdblockURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void AdblockURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void AdblockURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  target_client_->OnComplete(status);
}

void AdblockURLLoaderFactory::InProgressRequest::OnBindingsClosed() {
  factory_->RemoveRequest(this);
}

void AdblockURLLoaderFactory::InProgressRequest::OnClientDisconnected() {
  OnRequestError(net::ERR_ABORTED);
}

void AdblockURLLoaderFactory::InProgressRequest::Start(
    uint32_t options,
    network::ResourceRequest request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
    const absl::optional<GURL>& rewrite) {
  if (rewrite) {
    constexpr int kInternalRedirectStatusCode = net::HTTP_TEMPORARY_REDIRECT;
    net::RedirectInfo redirect_info = net::RedirectInfo::ComputeRedirectInfo(
        request.method, request.url, request.site_for_cookies,
        request.update_first_party_url_on_redirect
            ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
            : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
        request.referrer_policy, request.referrer.spec(),
        kInternalRedirectStatusCode, rewrite.value(),
        absl::nullopt /* referrer_policy_header */,
        false /* insecure_scheme_was_upgraded */, false /* copy_fragment */,
        false /* is_signed_exchange_fallback_redirect */);
    redirect_info.bypass_redirect_checks = true;

    auto head = network::mojom::URLResponseHead::New();
    std::string headers = base::StringPrintf(
        "HTTP/1.1 %i Internal Redirect\n"
        "Location: %s\n"
        "Non-Authoritative-Reason: ABPC\n\n",
        kInternalRedirectStatusCode, rewrite.value().spec().c_str());
    head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers));
    head->encoded_data_length = 0;

    // Close the connection with the current URLLoader and inform the
    // client about redirect. New URLLoader will be recreated after redirect.
    client_receiver_.reset();
    target_loader_.reset();
    target_client_->OnReceiveRedirect(redirect_info, std::move(head));
    return;
  }

  if (!factory_->target_factory_.is_bound()) {
    DLOG(WARNING)
        << "[eyeo] AdblockURLLoaderFactory::InProgressRequest::Start: "
           "target_factory_ not bound";
    return;
  }

  if (ShouldIgnoreRequest()) {
    VLOG(1) << "[eyeo] Unsupported scheme, allowing to load.";
    factory_->target_factory_->CreateLoaderAndStart(
        std::move(target_loader), request_id_, options, request,
        std::move(proxy_client), traffic_annotation);
    return;
  }

  VLOG(1) << "[eyeo] Checking filter match for: " << request.url << " ("
          << request.resource_type << ")";

  CheckFilterMatch(base::BindOnce(
      &InProgressRequest::OnRequestFilterMatchResult,
      weak_factory_.GetWeakPtr(), std::move(target_loader), options, request,
      std::move(proxy_client), traffic_annotation));
}

void AdblockURLLoaderFactory::InProgressRequest::OnRequestFilterMatchResult(
    ::mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
    uint32_t options,
    const network::ResourceRequest& request,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    FilterMatchResult result) {
  if (!factory_->target_factory_.is_bound()) {
    DLOG(WARNING) << "[eyeo] "
                     "AdblockURLLoaderFactory::InProgressRequest::"
                     "OnRequestFilterMatchResult: target_factory_ not bound";
    return;
  }
  if (result == FilterMatchResult::kBlockRule) {
    OnRequestError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }
  factory_->target_factory_->CreateLoaderAndStart(
      std::move(target_loader), request_id_, options, request,
      std::move(proxy_client), traffic_annotation);
}

void AdblockURLLoaderFactory::InProgressRequest::SetPreCommitUrlForFrame() {
  content::RenderFrameHost* host =
      content::RenderFrameHost::FromID(factory_->host_id_);
  if (adblock_resource_type_ == ContentType::Subdocument && host) {
    // Frames with HTML may trigger fetches of subresources from the renderer
    // process before the browser process (this one) receives a notification
    // about the navigation becoming committed. This could lead to a situation
    // where the FrameHierarchyBuilder cannot establish the URL of this frame
    // via GetLastCommittedURL(). We therefore set the "temporary" pre-commit
    // URL into an EyeoDocumentInfo instance associated with this frame.
    // This instance of EyeoDocumentInfo will be destroyed and re-created once
    // the navigation becomes committed, so it may be very short-lived and only
    // matter for the few subresource loads that happen during the HTML preload
    // phase.
    EyeoDocumentInfo* document_info =
        EyeoDocumentInfo::GetOrCreateForCurrentDocument(host);
    document_info->SetPreCommitURL(request_url_);
  }
}

bool AdblockURLLoaderFactory::InProgressRequest::ShouldIgnoreRequest() const {
  return !request_url_.SchemeIsHTTPOrHTTPS() && !request_url_.SchemeIsWSOrWSS();
}

AdblockURLLoaderFactory::AdblockURLLoaderFactory(
    AdblockURLLoaderFactoryConfig config,
    content::GlobalRenderFrameHostId host_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    std::string user_agent_string,
    DisconnectCallback on_disconnect)
    : config_(std::move(config)),
      host_id_(host_id),
      user_agent_string_(std::move(user_agent_string)),
      on_disconnect_(std::move(on_disconnect)) {
  DCHECK(config_.subscription_service);
  DCHECK(config_.resource_classifier);
  DCHECK(config_.element_hider);
  DCHECK(config_.sitekey_storage);
  DCHECK(config_.csp_injector);
  target_factory_.Bind(std::move(target_factory));
  target_factory_.set_disconnect_handler(base::BindOnce(
      &AdblockURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));
  proxy_receivers_.Add(this, std::move(receiver));
  proxy_receivers_.set_disconnect_handler(base::BindRepeating(
      &AdblockURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));
}

AdblockURLLoaderFactory::~AdblockURLLoaderFactory() = default;

void AdblockURLLoaderFactory::CreateLoaderAndStart(
    ::mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  requests_.insert(std::make_unique<InProgressRequest>(
      this, std::move(loader), request_id, options, request, std::move(client),
      traffic_annotation));
}

void AdblockURLLoaderFactory::Clone(
    ::mojo::PendingReceiver<URLLoaderFactory> factory) {
  proxy_receivers_.Add(this, std::move(factory));
}

bool AdblockURLLoaderFactory::CheckHostValid() const {
  return content::RenderFrameHost::FromID(host_id_) != nullptr;
}

void AdblockURLLoaderFactory::OnTargetFactoryError() {
  target_factory_.reset();
  proxy_receivers_.Clear();
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::OnProxyBindingError() {
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::RemoveRequest(InProgressRequest* request) {
  auto it = requests_.find(request);
  DCHECK(it != requests_.end());
  requests_.erase(it);
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::MaybeDestroySelf() {
  if (proxy_receivers_.empty() && requests_.empty()) {
    std::move(on_disconnect_).Run(this);
  }
}

}  // namespace adblock
