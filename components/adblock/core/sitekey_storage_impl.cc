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

#include "components/adblock/core/sitekey_storage_impl.h"

#include "absl/types/optional.h"
#include "base/base64.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "crypto/openssl_util.h"
#include "crypto/signature_verifier.h"
#include "net/http/http_response_headers.h"

namespace adblock {
namespace {

SiteKey GetSitekeyHeader(
    const scoped_refptr<net::HttpResponseHeaders>& headers) {
  size_t iterator = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&iterator, &name, &value)) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (name == adblock::kSiteKeyHeaderKey) {
      return SiteKey{value};
    }
  }
  return {};
}

}  // namespace

SitekeyStorageImpl::SitekeyStorageImpl() = default;

SitekeyStorageImpl::~SitekeyStorageImpl() = default;

void SitekeyStorageImpl::ProcessResponseHeaders(
    const GURL& request_url,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& user_agent) {
  if (user_agent.empty()) {
    LOG(WARNING) << "[eyeo] No user agent info";
    return;
  }

  auto site_key = GetSitekeyHeader(headers);
  if (site_key.value().empty()) {
    VLOG(1) << "[eyeo] No site key header";
    return;
  }

  ProcessSiteKey(request_url, site_key, user_agent);
}

absl::optional<std::pair<GURL, SiteKey>>
SitekeyStorageImpl::FindSiteKeyForAnyUrl(const std::vector<GURL>& urls) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& url : urls) {
    auto elem = url_to_sitekey_map_.find(url);
    if (elem != url_to_sitekey_map_.cend()) {
      return {*elem};
    }
  }
  return {};
}

void SitekeyStorageImpl::ProcessSiteKey(const GURL& request_url,
                                        const SiteKey& site_key,
                                        const std::string& user_agent) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!site_key.value().empty());
  auto site_key_pair = FindSiteKeyForAnyUrl({request_url});
  if (site_key_pair.has_value() &&
      site_key.value() == site_key_pair->second.value()) {
    DVLOG(3) << "[eyeo] Public key already stored for url: "
             << site_key_pair->first;
    return;
  }

  GURL url = request_url.GetAsReferrer();
  TRACE_EVENT1("eyeo", "ProcessSiteKeyImpl", "url", request_url.spec());
  size_t delimiter = site_key.value().find("_");
  if ((delimiter == std::string::npos) ||
      (delimiter >= (site_key.value().length() - 1))) {
    LOG(ERROR) << "[eyeo] Wrong format of site key header value: "
               << site_key.value();
    return;
  }

  std::string public_key = site_key.value().substr(0, delimiter);
  std::string public_key_stripped = public_key.substr(0, public_key.find("=="));
  std::string signature = site_key.value().substr(delimiter + 1);
  DVLOG(1) << "[eyeo] Found site key header, public key: " << public_key
           << ", signature: " << signature;

  auto path_with_query = url.path();
  if (url.has_query()) {
    path_with_query += "?" + url.query();
  }
  DLOG(INFO) << "[eyeo] Calling IsSitekeySignatureValid(publicKey, signature, "
                "uri, host,"
                " userAgent) with arguments: ("
             << public_key << ", " << signature << ", " << path_with_query
             << ", " << url.host() << ", " << user_agent << ")";

  std::string data = path_with_query + '\0' + url.host() + '\0' + user_agent;
  if (IsSitekeySignatureValid(public_key, signature, data) &&
      !request_url.is_empty() && request_url.is_valid() &&
      !site_key.value().empty()) {
    url_to_sitekey_map_[url] = SiteKey{public_key_stripped};
  } else {
    LOG(ERROR) << "[eyeo] Sitekey verification failed";
  }
}

bool SitekeyStorageImpl::IsSitekeySignatureValid(
    const std::string& public_key_b64,
    const std::string& signature_b64,
    const std::string& data) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string signature;
  if (!base::Base64Decode(signature_b64, &signature,
                          base::Base64DecodePolicy::kForgiving)) {
    DLOG(WARNING) << "[eyeo] Signature decode failed";
    return false;
  }

  std::string public_key;
  if (!base::Base64Decode(public_key_b64, &public_key,
                          base::Base64DecodePolicy::kForgiving)) {
    DLOG(WARNING) << "[eyeo] Public key decode failed";
    return false;
  }

  crypto::SignatureVerifier verifier;
  if (verifier.VerifyInit(crypto::SignatureVerifier::RSA_PKCS1_SHA1,
                          base::as_bytes(base::span(signature)),
                          base::as_bytes(base::span(public_key)))) {
    verifier.VerifyUpdate(base::as_bytes(base::span(data)));
    return verifier.VerifyFinal();
  }

  DLOG(WARNING) << "[eyeo] Verifier initialization failed";
  return false;
}

}  // namespace adblock
