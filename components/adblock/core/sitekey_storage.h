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

#ifndef COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_H_
#define COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_H_

#include <string>

#include "absl/types/optional.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace adblock {

/**
 * @brief Parses response headers in search for AdblockPlus sitekeys and stores
 * them.
 * Some filters can only be applied on pages that provide a valid sitekey.
 * Storage is not persistent.
 * Lives in the UI thread.
 */
class SitekeyStorage : public KeyedService {
 public:
  // Attempts to extract a sitekey from |headers|. If successful, the sitekey
  // is added to storage and can be retrieved by |FindSiteKeyForAnyUrl|.
  virtual void ProcessResponseHeaders(
      const GURL& request_url,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent) = 0;

  virtual absl::optional<std::pair<GURL, SiteKey>> FindSiteKeyForAnyUrl(
      const std::vector<GURL>& urls) const = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_H_
