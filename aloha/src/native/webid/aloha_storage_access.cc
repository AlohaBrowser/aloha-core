// Copyright 2026 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "aloha/src/native/webid/aloha_storage_access.h"

#include <tuple>
#include <utility>

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/cookie_manager.h"
#include "base/containers/flat_set.h"
#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/webid/federated_identity_auto_reauthn_permission_context_delegate.h"
#include "content/public/browser/webid/federated_identity_permission_context_delegate.h"
#include "net/base/schemeful_site.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom-shared.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace aloha {

namespace {

// Browser-owned registry of (profile, RP top-frame site, IdP site) triples
// for which a STORAGE_ACCESS_GRANT was actually issued this session. Written
// on the UI thread (permission decision), read on the IO thread (cookie
// enforcement) — hence the lock. The profile key is the profile's
// AwCookieAccessPolicy pointer (see GetProfileKey below), so a grant issued
// in one profile (Default) never validates statuses in another (Incognito).
//
// Entries are never removed: this mirrors Chrome's enforcement model, where
// the network service validates storage-access via STORAGE_ACCESS content
// settings keyed by (site, top-level site) with a TTL — not by document.
// Per-document scoping lives in Blink's storage-access bit, which the browser
// cannot verify on the network path anyway. The FedCM gates (sharing
// permission, user mediation) are re-checked on every new permission request,
// not here. In-memory only, matching InMemoryFederatedPermissionContext.
using GrantKey =
    std::tuple<const void*, net::SchemefulSite, net::SchemefulSite>;

base::Lock& GetGrantsLock() {
  static base::NoDestructor<base::Lock> lock;
  return *lock;
}

base::flat_set<GrantKey>& GetGrants() {
  static base::NoDestructor<base::flat_set<GrantKey>> grants;
  return *grants;
}

// The per-profile key shared by the record side (UI thread, from the
// BrowserContext) and the validation side (IO thread, where the cookie
// enforcement objects already hold the profile's AwCookieAccessPolicy*).
// Only reached for real AwBrowserContexts: all FedCM gates above the call
// site return DENIED first on test/other contexts (their federated identity
// contexts are null).
const void* GetProfileKey(content::BrowserContext* browser_context) {
  return static_cast<android_webview::AwBrowserContext*>(browser_context)
      ->GetCookieManager()
      ->cookie_access_policy();
}

void RecordGrant(const void* profile_key,
                 const url::Origin& rp,
                 const url::Origin& idp) {
  base::AutoLock auto_lock(GetGrantsLock());
  GetGrants().insert(GrantKey(profile_key, net::SchemefulSite(rp),
                              net::SchemefulSite(idp)));
}

bool HasGrant(const void* profile_key,
              const net::SchemefulSite& top_frame_site,
              const net::SchemefulSite& idp_site) {
  base::AutoLock auto_lock(GetGrantsLock());
  return GetGrants().contains(
      GrantKey(profile_key, top_frame_site, idp_site));
}

}  // namespace

blink::mojom::PermissionStatus GetStorageAccessPermissionStatus(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    StorageAccessCheckReason reason) {
  if (!browser_context) {
    return blink::mojom::PermissionStatus::DENIED;
  }
  // Chrome gate 1: the RP must have delegated FedCM to the requesting iframe
  // via the `identity-credentials-get` permissions policy. Skipped only when
  // there is no frame to consult (frameless status queries); the grant path
  // (RequestPermissionsFromCurrentDocument) always has a frame.
  if (render_frame_host &&
      !render_frame_host->IsFeatureEnabled(
          network::mojom::PermissionsPolicyFeature::kIdentityCredentialsGet)) {
    return blink::mojom::PermissionStatus::DENIED;
  }
  content::FederatedIdentityPermissionContextDelegate* fedcm_context =
      browser_context->GetFederatedIdentityPermissionContext();
  if (!fedcm_context) {
    return blink::mojom::PermissionStatus::DENIED;
  }
  // Chrome gate 2: FedCM sharing permission for this (RP, IdP) pair.
  // The FedCM request is made by the top-level RP page, so the sharing
  // permission is recorded with requester == embedder (see
  // RequestService::MarkUserAsSignedIn -> GrantSharingPermission(origin(),
  // GetEmbeddingOrigin(), idp)). The Storage Access request comes from the
  // IdP iframe: requesting = IdP origin, embedding = RP top-level origin.
  const url::Origin rp = url::Origin::Create(embedding_origin);
  const url::Origin idp = url::Origin::Create(requesting_origin);
  if (!fedcm_context->HasSharingPermission(rp, rp, idp)) {
    return blink::mojom::PermissionStatus::DENIED;
  }
  // Chrome gate 3: preventSilentAccess() / FedCM disconnect flag the RP as
  // requiring user mediation; the auto-grant must stop until the next
  // explicit sign-in.
  content::FederatedIdentityAutoReauthnPermissionContextDelegate*
      auto_reauthn_context =
          browser_context->GetFederatedIdentityAutoReauthnPermissionContext();
  if (!auto_reauthn_context ||
      auto_reauthn_context->RequiresUserMediation(rp)) {
    return blink::mojom::PermissionStatus::DENIED;
  }
  // Record the browser-side decision so cookie enforcement can validate
  // renderer-supplied kAccessViaAPI claims against it, see
  // ValidateStorageAccessStatus(). Only an actual grant issuance is recorded:
  // a status query reports GRANTED without granting anything, and must not
  // arm the cookie path.
  if (reason == StorageAccessCheckReason::kPermissionRequest) {
    RecordGrant(GetProfileKey(browser_context), rp, idp);
  }
  return blink::mojom::PermissionStatus::GRANTED;
}

void ClearStorageAccessGrantsForProfile(const void* profile_key) {
  base::AutoLock auto_lock(GetGrantsLock());
  base::EraseIf(GetGrants(), [profile_key](const GrantKey& key) {
    return std::get<0>(key) == profile_key;
  });
}

void RecordStorageAccessGrantForTesting(const void* profile_key,
                                        const url::Origin& rp,
                                        const url::Origin& idp) {
  RecordGrant(profile_key, rp, idp);
}

net::StorageAccessApiStatus ValidateStorageAccessStatus(
    net::StorageAccessApiStatus status,
    const void* profile_key,
    const std::optional<url::Origin>& top_frame_origin,
    const GURL& url) {
  switch (status) {
    case net::StorageAccessApiStatus::kNone:
      return net::StorageAccessApiStatus::kNone;
    case net::StorageAccessApiStatus::kAccessViaAPI:
      break;
  }
  if (!profile_key || !top_frame_origin || top_frame_origin->opaque()) {
    return net::StorageAccessApiStatus::kNone;
  }
  return HasGrant(profile_key, net::SchemefulSite(*top_frame_origin),
                  net::SchemefulSite(url))
             ? net::StorageAccessApiStatus::kAccessViaAPI
             : net::StorageAccessApiStatus::kNone;
}

}  // namespace aloha
