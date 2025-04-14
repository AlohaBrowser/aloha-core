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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_IMPL_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/net/adblock_resource_request.h"
#include "components/adblock/core/subscription/recommended_subscription_installer.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"

class PrefService;

namespace adblock {

class RecommendedSubscriptionInstallerImpl final
    : public RecommendedSubscriptionInstaller {
 public:
  using ResourceRequestMaker =
      base::RepeatingCallback<std::unique_ptr<AdblockResourceRequest>()>;

  RecommendedSubscriptionInstallerImpl(
      PrefService* pref_service,
      FilteringConfiguration* configuration,
      SubscriptionPersistentMetadata* persistent_metadata,
      ResourceRequestMaker request_maker);
  ~RecommendedSubscriptionInstallerImpl() final;

  void RunUpdateCheck() final;
  void RemoveAutoInstalledSubscriptions() final;

 private:
  bool IsUpdateDue() const;
  void OnRecommendationListDownloaded(
      const GURL& url,
      base::FilePath downloaded_file,
      scoped_refptr<net::HttpResponseHeaders> headers);
  void OnRecommendedSubscriptionsParsed(
      const std::vector<GURL>& recommended_subscriptions);

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<PrefService> pref_service_;
  raw_ptr<FilteringConfiguration> configuration_;
  raw_ptr<SubscriptionPersistentMetadata> persistent_metadata_;
  ResourceRequestMaker request_maker_;
  RecommendedSubscriptionsParsedCallback
      on_recommended_subscriptions_available_;
  std::unique_ptr<AdblockResourceRequest> resource_request_;
  base::WeakPtrFactory<RecommendedSubscriptionInstallerImpl> weak_ptr_factory_{
      this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_RECOMMENDED_SUBSCRIPTION_INSTALLER_IMPL_H_
