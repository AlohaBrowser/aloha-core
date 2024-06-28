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

#include "components/adblock/core/subscription/subscription_service_impl.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/parameter_pack.h"
#include "base/ranges/algorithm.h"
#include "base/trace_event/common/trace_event_common.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/subscription/filtering_configuration_maintainer.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_service.h"

namespace adblock {

class EmptySubscription : public Subscription {
 public:
  explicit EmptySubscription(const GURL& url) : url_(url) {}
  GURL GetSourceUrl() const override { return url_; }
  std::string GetTitle() const override { return ""; }
  std::string GetCurrentVersion() const override { return ""; }
  InstallationState GetInstallationState() const override {
    return InstallationState::Unknown;
  }
  base::Time GetInstallationTime() const override {
    return base::Time::UnixEpoch();
  }
  base::TimeDelta GetExpirationInterval() const override {
    return base::TimeDelta();
  }

 private:
  ~EmptySubscription() override {}
  const GURL url_;
};

SubscriptionServiceImpl::SubscriptionServiceImpl(
    FilteringConfigurationMaintainerFactory maintainer_factory,
    FilteringConfigurationCleaner configuration_cleaner)
    : maintainer_factory_(std::move(maintainer_factory)),
      configuration_cleaner_(std::move(configuration_cleaner)) {}

SubscriptionServiceImpl::~SubscriptionServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& entry : maintainers_) {
    entry.first->RemoveObserver(this);
  }
}

std::vector<scoped_refptr<Subscription>>
SubscriptionServiceImpl::GetCurrentSubscriptions(
    FilteringConfiguration* configuration) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = base::ranges::find_if(maintainers_, [&](const auto& entry) {
    return entry.first.get() == configuration;
  });
  DCHECK(it != maintainers_.end()) << "Cannot get Subscriptions from an "
                                      "unregistered FilteringConfiguration";

  // First get the list from FilteringConfiguration which represents actual
  // settings state but misses subscription metadata (it's just a list of urls).
  auto urls = it->first->GetFilterLists();
  std::vector<scoped_refptr<adblock::Subscription>> result;
  base::ranges::transform(urls, std::back_inserter(result),
                          [](const auto& url) {
                            return base::MakeRefCounted<EmptySubscription>(url);
                          });
  if (it->second) {
    // As the list from FilteringConfiguration is lacking metadata, replace
    // each entry from FilteringConfiguration by respective entry from
    // maintainer, leaving entries from FilteringConfiguration if there is no
    // counterpart in maintainer (this can be the case when subscription storage
    // is not yet initialized).
    auto maintainer_filter_lists = it->second->GetCurrentSubscriptions();
    for (size_t i = 0; i < result.size(); ++i) {
      auto list_it = base::ranges::find_if(
          maintainer_filter_lists, [&](const auto& entry) {
            return entry->GetSourceUrl() == result[i]->GetSourceUrl();
          });
      if (list_it != maintainer_filter_lists.end()) {
        result[i] = *list_it;
      }
    }
  }
  return result;
}

void SubscriptionServiceImpl::InstallFilteringConfiguration(
    std::unique_ptr<FilteringConfiguration> configuration) {
  auto name = configuration->GetName();
  auto it = base::ranges::find_if(maintainers_, [&name](const auto& entry) {
    return entry.first.get()->GetName() == name;
  });
  if (it != maintainers_.end()) {
    LOG(WARNING)
        << "[eyeo] Trying to install configuration with duplicated name: "
        << name;
    return;
  }
  VLOG(1) << "[eyeo] FilteringConfiguration installed: "
          << configuration->GetName();
  configuration->AddObserver(this);
  std::unique_ptr<FilteringConfigurationMaintainer> maintainer;
  if (configuration->IsEnabled()) {
    // Only enabled configurations should be maintained. Disabled configurations
    // are observed and added to the collection, but a Maintainer will be
    // created in OnEnabledStateChanged.
    maintainer = MakeMaintainer(configuration.get());
  }
  auto* ptr = configuration.get();
  maintainers_.insert(
      std::make_pair(std::move(configuration), std::move(maintainer)));
  for (auto& observer : observers_) {
    observer.OnFilteringConfigurationInstalled(ptr);
  }
}

void SubscriptionServiceImpl::UninstallFilteringConfiguration(
    std::string_view configuration_name) {
  auto it = base::ranges::find_if(maintainers_, [&](const auto& entry) {
    return entry.first.get()->GetName() == configuration_name;
  });
  if (it == maintainers_.end()) {
    LOG(WARNING) << "[eyeo] Trying to uninstall non existing configuration: "
                 << configuration_name;
    return;
  }
  VLOG(1) << "[eyeo] FilteringConfiguration uninstalled: "
          << configuration_name;
  it->first->RemoveObserver(this);
  it->second.reset();
  configuration_cleaner_.Run(it->first.get());
  maintainers_.erase(it);
  for (auto& observer : observers_) {
    observer.OnFilteringConfigurationUninstalled(configuration_name);
  }
}

std::vector<FilteringConfiguration*>
SubscriptionServiceImpl::GetInstalledFilteringConfigurations() {
  std::vector<FilteringConfiguration*> result;
  base::ranges::transform(maintainers_, std::back_inserter(result),
                          [](const auto& pair) { return pair.first.get(); });
  return result;
}

FilteringConfiguration* SubscriptionServiceImpl::GetFilteringConfiguration(
    std::string_view configuration_name) const {
  const auto it = base::ranges::find_if(
      maintainers_, [&configuration_name](const auto& pair) {
        return pair.first->GetName() == configuration_name;
      });
  if (it == maintainers_.end()) {
    LOG(WARNING)
        << "[eyeo] Trying to get a pointer to not installed configuration "
        << configuration_name;
    return nullptr;
  }
  return it->first.get();
}

SubscriptionService::Snapshot SubscriptionServiceImpl::GetCurrentSnapshot()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Snapshot snapshot;
  for (const auto& entry : maintainers_) {
    if (!entry.second) {
      continue;  // Configuration is disabled
    }
    snapshot.push_back(entry.second->GetSubscriptionCollection());
  }
  return snapshot;
}

void SubscriptionServiceImpl::AddObserver(SubscriptionObserver* o) {
  observers_.AddObserver(o);
}

void SubscriptionServiceImpl::RemoveObserver(SubscriptionObserver* o) {
  observers_.RemoveObserver(o);
}

void SubscriptionServiceImpl::OnEnabledStateChanged(
    FilteringConfiguration* config) {
  auto it = base::ranges::find_if(maintainers_, [&](const auto& entry) {
    return entry.first.get() == config;
  });
  DCHECK(it != maintainers_.end()) << "Received OnEnabledStateChanged from "
                                      "unregistered FilteringConfiguration";
  VLOG(1) << "[eyeo] FilteringConfiguration " << config->GetName()
          << (config->IsEnabled() ? " enabled" : " disabled");
  if (config->IsEnabled()) {
    // Enable the configuration by creating a new
    // FilteringConfigurationMaintainer. This triggers installing missing
    // subscriptions etc.
    it->second = MakeMaintainer(config);
  } else {
    // Disable the configuration by removing its
    // FilteringConfigurationMaintainer. This cancels all related operations and
    // frees all associated memory.
    it->second.reset();
  }
}

void SubscriptionServiceImpl::OnSubscriptionUpdated(
    const GURL& subscription_url) {
  for (auto& observer : observers_) {
    observer.OnSubscriptionInstalled(subscription_url);
  }
}

std::unique_ptr<FilteringConfigurationMaintainer>
SubscriptionServiceImpl::MakeMaintainer(FilteringConfiguration* configuration) {
  return maintainer_factory_.Run(
      configuration,
      base::BindRepeating(&SubscriptionServiceImpl::OnSubscriptionUpdated,
                          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace adblock
