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

#ifndef COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_IMPL_H_

#include <map>

#include "absl/types/optional.h"

#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/sitekey_storage.h"

namespace adblock {

class SitekeyStorageImpl final : public SitekeyStorage {
 public:
  SitekeyStorageImpl();
  ~SitekeyStorageImpl() final;

  void ProcessResponseHeaders(
      const GURL& request_url,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent) final;

  absl::optional<std::pair<GURL, SiteKey>> FindSiteKeyForAnyUrl(
      const std::vector<GURL>& urls) const final;

 private:
  void ProcessSiteKey(const GURL& request_url,
                      const SiteKey& site_key,
                      const std::string& user_agent);

  bool IsSitekeySignatureValid(const std::string& public_key_b64,
                               const std::string& signature_b64,
                               const std::string& data) const;

  SEQUENCE_CHECKER(sequence_checker_);
  std::map<GURL, SiteKey> url_to_sitekey_map_;
  base::WeakPtrFactory<SitekeyStorageImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SITEKEY_STORAGE_IMPL_H_
