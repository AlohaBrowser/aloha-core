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

#ifndef ALOHA_SRC_NATIVE_WEBID_ALOHA_STORAGE_ACCESS_H_
#define ALOHA_SRC_NATIVE_WEBID_ALOHA_STORAGE_ACCESS_H_

#include <optional>

#include "base/notreached.h"
#include "net/storage_access_api/status.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"
#include "url/origin.h"

class GURL;

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace aloha {

// ALOHA https://app.clickup.com/t/86ev1nrnd
// FedCM <-> Storage Access API integration for WebView.
//
// Chrome auto-grants document.requestStorageAccess() for an IdP origin that
// the user has just connected to the embedding site via FedCM
// (StorageAccessGrantPermissionContext + FedCmWithStorageAccessAPI). That
// bridge lives in chrome/browser/, which WebView does not have, so stock
// WebView denies STORAGE_ACCESS_GRANT unconditionally. Without it, legacy
// cookie-based IdP machinery (e.g. the deprecated gapi.auth2 relay iframe
// accounts.google.com/o/oauth2/iframe) never regains cookie access after a
// successful FedCM sign-in, and such sites never observe the session.
//
// This file is the single home of the Bromium-side logic; upstream files
// carry only one-line hooks into it.

// Why the permission status is being computed. Only an actual permission
// request (document.requestStorageAccess()) records the issued grant into the
// cookie-validation registry; a status query (permissions.query etc.) must
// not — it reports what WOULD be granted without granting anything.
enum class StorageAccessCheckReason {
  kPermissionRequest,
  kStatusQuery,
};

// Returns GRANTED iff, mirroring Chrome's IsAutograntViaFedCmAllowed
// (chrome/browser/storage_access_api/storage_access_grant_permission_context.cc):
//   1. `render_frame_host` (when non-null; null on frameless status queries)
//      has the `identity-credentials-get` permissions policy enabled, i.e. the
//      RP delegated FedCM to this iframe;
//   2. `browser_context`'s FederatedIdentityPermissionContext holds a FedCM
//      sharing permission for the pair (RP = embedding_origin,
//      IdP = requesting_origin);
//   3. the RP does not require user mediation (preventSilentAccess() /
//      FedCM disconnect set that bit and must revoke the auto-grant).
// DENIED otherwise.
// Divergence from Chrome (content-public delegate limitation): the sharing
// permission lookup is exact-origin (rp, rp, idp), while Chrome matches by
// SchemefulSite with the requester ignored. Subdomain drift between the FedCM
// config origin and the SAA iframe origin therefore misses here.
// Hooked from AwPermissionManager for PermissionType::STORAGE_ACCESS_GRANT.
blink::mojom::PermissionStatus GetStorageAccessPermissionStatus(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    StorageAccessCheckReason reason);

// Validates a renderer-supplied StorageAccessApiStatus against the
// browser-owned registry of issued grants before it may influence cookie
// policy. `storage_access_api_status` arrives over mojo
// (RestrictedCookieManager methods) or in ResourceRequest, i.e. a compromised
// renderer can claim kAccessViaAPI at will; without this check that claim
// alone would bypass setAcceptThirdPartyCookies(false) — the exact hole
// upstream closed in crrev.com/c/7726801. Returns kAccessViaAPI only when the
// browser actually issued a STORAGE_ACCESS_GRANT for the pair
// (site of `top_frame_origin`, site of `url`) in this session (recorded by
// GetStorageAccessPermissionStatus above); kNone otherwise.
// `top_frame_origin` must come from browser-owned data (net::IsolationInfo
// captured at factory/RCM creation), never from renderer-supplied fields.
// `profile_key` scopes the lookup to one profile: pass the profile's
// AwCookieAccessPolicy pointer — the browser-owned per-profile object that
// every cookie-enforcement site already holds (RCM proxy member, loader
// factory member); a grant issued in one profile never validates statuses in
// another (Default vs Incognito).
// Callable on any thread (lock-protected registry); call only when the
// incoming status is kAccessViaAPI to keep the common path free.
net::StorageAccessApiStatus ValidateStorageAccessStatus(
    net::StorageAccessApiStatus status,
    const void* profile_key,
    const std::optional<url::Origin>& top_frame_origin,
    const GURL& url);

// Removes every recorded grant keyed by `profile_key`. Called from
// AwCookieAccessPolicy's destructor so that a later profile allocation
// reusing the same address can never inherit another profile's grants
// (profiles can be deleted at runtime via androidx ProfileStore).
void ClearStorageAccessGrantsForProfile(const void* profile_key);

// Test-only: records a grant directly, bypassing the FedCM gates.
void RecordStorageAccessGrantForTesting(const void* profile_key,
                                        const url::Origin& rp,
                                        const url::Origin& idp);

// Whether a cookie access with this StorageAccessApiStatus must be treated as
// first-party-like (unpartitioned) despite third-party cookies being blocked.
// True for kAccessViaAPI. The status MUST have been passed through
// ValidateStorageAccessStatus() at the mojo/ResourceRequest boundary first.
// Hooked from AwCookieAccessPolicy::CanAccessCookies on the IO-thread hot
// path, hence inline.
inline bool StatusAllowsUnpartitionedCookies(
    net::StorageAccessApiStatus status) {
  switch (status) {
    case net::StorageAccessApiStatus::kNone:
      return false;
    case net::StorageAccessApiStatus::kAccessViaAPI:
      return true;
  }
  NOTREACHED();  // Out-of-range value from a compromised renderer.
}

}  // namespace aloha

#endif  // ALOHA_SRC_NATIVE_WEBID_ALOHA_STORAGE_ACCESS_H_
