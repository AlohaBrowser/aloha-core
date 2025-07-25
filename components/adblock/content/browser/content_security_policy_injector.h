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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_H_

#include <string>

#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/request_initiator.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/global_routing_id.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/parsed_headers.mojom-forward.h"
#include "url/gurl.h"

namespace adblock {

using InsertContentSecurityPolicyHeadersCallback =
    base::OnceCallback<void(network::mojom::ParsedHeadersPtr)>;

// Implements CSP filter application.
//
// CSP filters are an anti-circumvention technique that allows injecting a
// Content Security Policy header into a HTTP response.
// For example, a "Content-Security-Policy: script-src: 'none'" header blocks
// all scripts, including inline, in the received document.
// This will not block downloading the resource, but may stop it from executing
// further actions once it has downloaded.
class ContentSecurityPolicyInjector : public KeyedService {
 public:
  // If a CSP filter exists for this URL in any of currently installed filter
  // lists, inserts its payload into |headers| as a Content-Security-Policy
  // header type.
  // If |headers| were changed, |callback| will receive a ParsedHeaders object
  // that matches the new state of the response headers - otherwise |callback|
  // will receive nullptr.
  virtual void InsertContentSecurityPolicyHeadersIfApplicable(
      const GURL& request_url,
      const RequestInitiator& request_initiator,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      InsertContentSecurityPolicyHeadersCallback callback) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_CONTENT_SECURITY_POLICY_INJECTOR_H_
