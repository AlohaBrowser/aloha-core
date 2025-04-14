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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTENT_BROWSER_CLIENT_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTENT_BROWSER_CLIENT_H_

#include "build/buildflag.h"
#include "components/adblock/content/browser/adblock_context_data.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/adblock_web_ui_controller_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "url/url_util.h"

#ifdef EYEO_INTERCEPT_DEBUG_URL
#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#endif

#include "extensions/buildflags/buildflags.h"
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_util.h"
#endif

#include "components/adblock/content/browser/adblock_internals_ui.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"

namespace adblock {

/**
 * @brief Intercepts network and UI events to inject ad-filtering.
 * Provides ad-filtering implementations of URLLoaderThrottles.
 * Binds a mojo connection between Renderer processes and the
 * Browser-process-based ResourceClassificationRunner.
 * Lives in browser process UI thread.
 */
template <class ContentBrowserClientBase>
class AdblockContentBrowserClient : public ContentBrowserClientBase {
 public:
  template <typename... Args>
  explicit AdblockContentBrowserClient(Args&&... args)
      : ContentBrowserClientBase(std::forward<Args>(args)...) {}

  AdblockContentBrowserClient(const AdblockContentBrowserClient&) = delete;
  AdblockContentBrowserClient& operator=(const AdblockContentBrowserClient&) =
      delete;
  ~AdblockContentBrowserClient() override = default;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Enable ad filtering also for requests initiated by extensions.
  // This allows implementing extension-driven browser tests.
  // In production code, requests from extensions are not blocked.
  static void ForceAdblockProxyForTesting();
#endif

  bool WillInterceptWebSocket(content::RenderFrameHost* frame) override;
  void CreateWebSocket(
      content::RenderFrameHost* frame,
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client) override;
  void WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      content::ContentBrowserClient::URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      const net::IsolationInfo& isolation_info,
      absl::optional<int64_t> navigation_id,
      ukm::SourceIdObj ukm_source_id,
      network::URLLoaderFactoryBuilder& factory_builder,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks,
      bool* disable_secure_dns,
      network::mojom::URLLoaderFactoryOverridePtr* factory_override,
      scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner)
      override;

  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override;

 protected:
  virtual bool IsFilteringNeeded(content::RenderFrameHost* frame,
                                 content::BrowserContext* browser_context);

  // current_browser_context is the BrowserContext relevant for the currently
  // processed request. It might be an off-the-record browser context. This
  // method allows implementing embedder-specific logic for getting the "right"
  // BrowserContext for eyeo services, which might be the "default" or the
  // "original" BrowserContext, depending on platform.
  virtual content::BrowserContext* GetBrowserContextForEyeoFactories(
      content::BrowserContext* current_browser_context) = 0;

 private:
  content::BrowserContext* GetBrowserContext(content::RenderFrameHost* frame) {
    DCHECK(frame);
    return GetBrowserContextForEyeoFactories(
        frame->GetProcess()->GetBrowserContext());
  }

  void OnWebSocketFilterCheckCompleted(
      content::GlobalRenderFrameHostId render_frame_host_id,
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      adblock::FilterMatchResult result);

  base::WeakPtrFactory<AdblockContentBrowserClient<ContentBrowserClientBase>>
      weak_factory_{this};

#if BUILDFLAG(ENABLE_EXTENSIONS)
  static bool force_adblock_proxy_for_testing_;
#endif
};

#if BUILDFLAG(ENABLE_EXTENSIONS)
// static
template <class ContentBrowserClientBase>
bool AdblockContentBrowserClient<
    ContentBrowserClientBase>::force_adblock_proxy_for_testing_ = false;

// static
template <class ContentBrowserClientBase>
void AdblockContentBrowserClient<
    ContentBrowserClientBase>::ForceAdblockProxyForTesting() {
  force_adblock_proxy_for_testing_ = true;
}
#endif

template <class ContentBrowserClientBase>
bool AdblockContentBrowserClient<ContentBrowserClientBase>::
    WillInterceptWebSocket(content::RenderFrameHost* frame) {
  if (frame && IsFilteringNeeded(frame, GetBrowserContext(frame))) {
    return true;
  }
  return ContentBrowserClientBase::WillInterceptWebSocket(frame);
}

template <class ContentBrowserClientBase>
void AdblockContentBrowserClient<ContentBrowserClientBase>::CreateWebSocket(
    content::RenderFrameHost* frame,
    content::ContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  if (frame && IsFilteringNeeded(frame, GetBrowserContext(frame))) {
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            GetBrowserContext(frame));
    auto* classification_runner =
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            GetBrowserContext(frame));
    classification_runner->CheckRequestFilterMatch(
        subscription_service->GetCurrentSnapshot(), url, ContentType::Websocket,
        RequestInitiator(frame),
        base::BindOnce(
            &AdblockContentBrowserClient<
                ContentBrowserClientBase>::OnWebSocketFilterCheckCompleted,
            weak_factory_.GetWeakPtr(), frame->GetGlobalId(),
            std::move(factory), url, site_for_cookies, user_agent,
            std::move(handshake_client)));
  } else {
    DCHECK(ContentBrowserClientBase::WillInterceptWebSocket(frame));
    ContentBrowserClientBase::CreateWebSocket(frame, std::move(factory), url,
                                              site_for_cookies, user_agent,
                                              std::move(handshake_client));
  }
}

template <class ContentBrowserClientBase>
void AdblockContentBrowserClient<ContentBrowserClientBase>::
    RegisterBrowserInterfaceBindersForFrame(
        content::RenderFrameHost* render_frame_host,
        mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  ContentBrowserClientBase::RegisterBrowserInterfaceBindersForFrame(
      render_frame_host, map);
  content::RegisterWebUIControllerInterfaceBinder<
      ::mojom::adblock_internals::AdblockInternalsPageHandler,
      adblock::AdblockInternalsUI>(map);
}

template <class ContentBrowserClientBase>
void AdblockContentBrowserClient<ContentBrowserClientBase>::
    WillCreateURLLoaderFactory(
        content::BrowserContext* browser_context,
        content::RenderFrameHost* frame,
        int render_process_id,
        content::ContentBrowserClient::URLLoaderFactoryType type,
        const url::Origin& request_initiator,
        const net::IsolationInfo& isolation_info,
        absl::optional<int64_t> navigation_id,
        ukm::SourceIdObj ukm_source_id,
        network::URLLoaderFactoryBuilder& factory_builder,
        mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
            header_client,
        bool* bypass_redirect_checks,
        bool* disable_secure_dns,
        network::mojom::URLLoaderFactoryOverridePtr* factory_override,
        scoped_refptr<base::SequencedTaskRunner>
            navigation_response_task_runner) {
  // Create Chromium proxy first as WebRequestProxyingURLLoaderFactory logic
  // depends on being first proxy
  ContentBrowserClientBase::WillCreateURLLoaderFactory(
      browser_context, frame, render_process_id, type, request_initiator,
      isolation_info, navigation_id, ukm_source_id, factory_builder,
      header_client, bypass_redirect_checks, disable_secure_dns,
      factory_override, navigation_response_task_runner);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!force_adblock_proxy_for_testing_ &&
      request_initiator.scheme() == extensions::kExtensionScheme) {
    VLOG(1) << "[eyeo] Do not use adblock proxy for extensions requests "
               "[extension id:"
            << request_initiator.host() << "].";
    return;
  }
#endif
  auto* eyeo_browser_context =
      GetBrowserContextForEyeoFactories(browser_context);
  bool use_adblock_proxy =
      (type == content::ContentBrowserClient::URLLoaderFactoryType::
                   kDocumentSubResource ||
       type ==
           content::ContentBrowserClient::URLLoaderFactoryType::kNavigation ||
       type == content::ContentBrowserClient::URLLoaderFactoryType::
                   kServiceWorkerSubResource ||
       type == content::ContentBrowserClient::URLLoaderFactoryType::
                   kServiceWorkerScript) &&
      IsFilteringNeeded(frame, eyeo_browser_context);

  bool use_test_loader = false;
#ifdef EYEO_INTERCEPT_DEBUG_URL
  if (frame) {
    content::WebContents* wc = content::WebContents::FromRenderFrameHost(frame);
    use_test_loader =
        (type ==
         content::ContentBrowserClient::URLLoaderFactoryType::kNavigation) &&
        wc->GetVisibleURL().is_valid() &&
        url::DomainIs(wc->GetVisibleURL().host_piece(),
                      AdblockURLLoaderFactoryForTest::kEyeoDebugDataHostName);
    use_adblock_proxy |= use_test_loader;
  }
#endif

  if (use_adblock_proxy) {
    auto [proxied_receiver, target_factory_remote] = factory_builder.Append();
    const RequestInitiator initiator =
        frame ? RequestInitiator(frame)
              : RequestInitiator(request_initiator.GetURL());
    AdblockContextData::StartProxying(
        eyeo_browser_context, initiator, std::move(proxied_receiver),
        std::move(target_factory_remote),
        ContentBrowserClientBase::GetUserAgent(), use_test_loader);
  }
}

template <class ContentBrowserClientBase>
void AdblockContentBrowserClient<ContentBrowserClientBase>::
    OnWebSocketFilterCheckCompleted(
        content::GlobalRenderFrameHostId render_frame_host_id,
        content::ContentBrowserClient::WebSocketFactory factory,
        const GURL& url,
        const net::SiteForCookies& site_for_cookies,
        const absl::optional<std::string>& user_agent,
        mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
            handshake_client,
        adblock::FilterMatchResult result) {
  auto* frame = content::RenderFrameHost::FromID(render_frame_host_id);
  if (!frame) {
    return;
  }
  const bool has_blocking_filter =
      result == adblock::FilterMatchResult::kBlockRule;
  if (!has_blocking_filter) {
    VLOG(1) << "[eyeo] Web socket allowed for " << url;
    if (ContentBrowserClientBase::WillInterceptWebSocket(frame)) {
      ContentBrowserClientBase::CreateWebSocket(frame, std::move(factory), url,
                                                site_for_cookies, user_agent,
                                                std::move(handshake_client));
      return;
    }

    std::vector<network::mojom::HttpHeaderPtr> headers;
    if (user_agent) {
      headers.push_back(network::mojom::HttpHeader::New(
          net::HttpRequestHeaders::kUserAgent, *user_agent));
    }
    std::move(factory).Run(url, std::move(headers), std::move(handshake_client),
                           mojo::NullRemote(), mojo::NullRemote());
  }

  VLOG(1) << "[eyeo] Web socket blocked for " << url;
}

template <class ContentBrowserClientBase>
bool AdblockContentBrowserClient<ContentBrowserClientBase>::IsFilteringNeeded(
    content::RenderFrameHost* frame,
    content::BrowserContext* browser_context) {
  if (browser_context) {
    return std::ranges::any_of(
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            browser_context)
            ->GetInstalledFilteringConfigurations(),
        &adblock::FilteringConfiguration::IsEnabled);
  }
  return false;
}

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_CONTENT_BROWSER_CLIENT_H_
