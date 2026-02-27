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

// ALOHA https://app.clickup.com/t/2558578/86eyfccqg: see header for the profile-dedup design.

#include "components/adblock/content/browser/shared_subscription_service_decorator.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"

namespace adblock {
namespace {

bool IsOwnedBy(SubscriptionService& service,
               FilteringConfiguration* configuration) {
  const auto configurations = service.GetInstalledFilteringConfigurations();
  return std::ranges::find(configurations, configuration) !=
         configurations.end();
}

bool HasConfigurationNamed(
    const std::vector<FilteringConfiguration*>& configurations,
    const std::string& name) {
  return std::ranges::any_of(
      configurations,
      [&](FilteringConfiguration* config) { return config->GetName() == name; });
}

}  // namespace

SharedSubscriptionServiceDecorator::SharedSubscriptionServiceDecorator(
    std::unique_ptr<SubscriptionService> wrapped,
    DefaultServiceGetter default_service_getter)
    : wrapped_(std::move(wrapped)),
      default_service_getter_(std::move(default_service_getter)) {
  DCHECK(wrapped_);
  wrapped_->AddObserver(this);
  // Resolve eagerly if possible; the Default context normally exists before
  // any non-default context. GetDefaultService() retries lazily otherwise.
  GetDefaultService();
}

SharedSubscriptionServiceDecorator::~SharedSubscriptionServiceDecorator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  StopObserving();
}

std::vector<scoped_refptr<Subscription>>
SharedSubscriptionServiceDecorator::GetCurrentSubscriptions(
    FilteringConfiguration* configuration) const {
  // The merged GetInstalledFilteringConfigurations() hands out both own and
  // Default-profile configurations - route to the service that actually owns
  // |configuration|. Passing a foreign pointer into SubscriptionServiceImpl
  // is undefined behavior (it dereferences a failed lookup in release).
  if (IsOwnedBy(*wrapped_, configuration)) {
    return wrapped_->GetCurrentSubscriptions(configuration);
  }
  auto* default_service = GetDefaultService();
  if (default_service && IsOwnedBy(*default_service, configuration)) {
    return default_service->GetCurrentSubscriptions(configuration);
  }
  LOG(WARNING) << "[eyeo] GetCurrentSubscriptions called with an unknown "
                  "FilteringConfiguration, returning an empty list";
  return {};
}

void SharedSubscriptionServiceDecorator::InstallFilteringConfiguration(
    std::unique_ptr<FilteringConfiguration> configuration) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Non-default profiles must never download or store filter lists: the
  // shared "adblock" collection is merged in from the Default profile via
  // GetCurrentSnapshot(). Strip filter lists instead of rejecting the whole
  // configuration - callers (e.g. FilteringConfigurationAndroid) keep a raw
  // pointer to |configuration| and dereference it right after installing.
  const auto filter_lists = configuration->GetFilterLists();
  if (!filter_lists.empty()) {
    LOG(WARNING) << "[eyeo] Removing " << filter_lists.size()
                 << " filter list(s) from configuration '"
                 << configuration->GetName()
                 << "' installed on a non-default profile: downloaded "
                    "subscriptions are shared from the Default profile.";
    for (const auto& url : filter_lists) {
      configuration->RemoveFilterList(url);
    }
  }
  wrapped_->InstallFilteringConfiguration(std::move(configuration));
}

void SharedSubscriptionServiceDecorator::UninstallFilteringConfiguration(
    std::string_view configuration_name) {
  wrapped_->UninstallFilteringConfiguration(configuration_name);
}

std::vector<FilteringConfiguration*>
SharedSubscriptionServiceDecorator::GetInstalledFilteringConfigurations() {
  // Includes the Default profile's configurations so that
  // IsFilteringNeeded() / IsAdblockEnabled() gates treat this profile as
  // filtering-enabled even when it has no configurations of its own (the
  // embedder configures adblock on the Default profile only). On a name
  // collision Default's configuration wins - consistently with
  // GetFilteringConfiguration() and with GetCurrentSnapshot(), where
  // Default's collection also wins (the colliding local stub carries no
  // downloaded lists and must not shadow the enabled state or the rules of
  // Default's config). Startup additionally purges such stale local
  // configurations, so collisions are transient at worst.
  auto* default_service = GetDefaultService();
  if (!default_service) {
    return wrapped_->GetInstalledFilteringConfigurations();
  }
  auto configurations = default_service->GetInstalledFilteringConfigurations();
  for (auto* own_configuration :
       wrapped_->GetInstalledFilteringConfigurations()) {
    if (!HasConfigurationNamed(configurations,
                               own_configuration->GetName())) {
      configurations.push_back(own_configuration);
    }
  }
  return configurations;
}

FilteringConfiguration*
SharedSubscriptionServiceDecorator::GetFilteringConfiguration(
    std::string_view configuration_name) const {
  // Consistent with the merged GetInstalledFilteringConfigurations():
  // Default's configuration wins on a name collision, local configurations
  // resolve on a Default miss. Without the Default resolution, name-based
  // callers (FilteringConfigurationAndroid) would create a NEW empty
  // persisted configuration on this profile, whose empty collection would
  // break document-allowlisting in the merged snapshot.
  auto* default_service = GetDefaultService();
  if (default_service) {
    auto* configuration =
        default_service->GetFilteringConfiguration(configuration_name);
    if (configuration) {
      return configuration;
    }
  }
  return wrapped_->GetFilteringConfiguration(configuration_name);
}

SubscriptionService::Snapshot
SharedSubscriptionServiceDecorator::GetCurrentSnapshot() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Snapshot own_snapshot = wrapped_->GetCurrentSnapshot();
  auto* default_service = GetDefaultService();
  if (!default_service) {
    return own_snapshot;
  }
  // Merge the Default profile's entire snapshot (downloaded lists and its
  // custom rules): filtering in non-default profiles must match Default
  // exactly, with no per-profile setup required from the embedder. On a name
  // collision (a stale local config persisted by older versions) Default's
  // collection wins: the local one carries no downloaded lists, and an extra
  // near-empty collection would break document-allowlisting, which requires
  // an allowing rule from EVERY collection in the snapshot.
  Snapshot default_snapshot = default_service->GetCurrentSnapshot();
  Snapshot snapshot;
  for (auto& own_collection : own_snapshot) {
    const bool shadowed_by_default = std::ranges::any_of(
        default_snapshot, [&](const auto& default_collection) {
          return default_collection->GetFilteringConfigurationName() ==
                 own_collection->GetFilteringConfigurationName();
        });
    if (!shadowed_by_default) {
      snapshot.push_back(std::move(own_collection));
    }
  }
  for (auto& default_collection : default_snapshot) {
    snapshot.push_back(std::move(default_collection));
  }
  return snapshot;
}

std::map<GURL, utils::DownloadStats>
SharedSubscriptionServiceDecorator::GetDownloadStats(
    std::string_view configuration_name) const {
  // Route by configuration ownership, consistent with
  // GetFilteringConfiguration(): Default wins on a name collision (real
  // download stats live on the Default profile).
  auto* default_service = GetDefaultService();
  if (default_service &&
      default_service->GetFilteringConfiguration(configuration_name)) {
    return default_service->GetDownloadStats(configuration_name);
  }
  return wrapped_->GetDownloadStats(configuration_name);
}

void SharedSubscriptionServiceDecorator::SetAutoInstallEnabled(bool enabled) {
  wrapped_->SetAutoInstallEnabled(enabled);
}

bool SharedSubscriptionServiceDecorator::IsAutoInstallEnabled() const {
  return wrapped_->IsAutoInstallEnabled();
}

void SharedSubscriptionServiceDecorator::AddObserver(
    SubscriptionObserver* observer) {
  observers_.AddObserver(observer);
}

void SharedSubscriptionServiceDecorator::RemoveObserver(
    SubscriptionObserver* observer) {
  observers_.RemoveObserver(observer);
}

void SharedSubscriptionServiceDecorator::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  StopObserving();
  wrapped_->Shutdown();
}

void SharedSubscriptionServiceDecorator::OnSubscriptionInstalled(
    const GURL& subscription_url) {
  for (auto& observer : observers_) {
    observer.OnSubscriptionInstalled(subscription_url);
  }
}

void SharedSubscriptionServiceDecorator::OnFilteringConfigurationInstalled(
    FilteringConfiguration* config) {
  for (auto& observer : observers_) {
    observer.OnFilteringConfigurationInstalled(config);
  }
}

void SharedSubscriptionServiceDecorator::OnFilteringConfigurationUninstalled(
    std::string_view config_name) {
  for (auto& observer : observers_) {
    observer.OnFilteringConfigurationUninstalled(config_name);
  }
}

SubscriptionService* SharedSubscriptionServiceDecorator::GetDefaultService()
    const {
  if (!default_service_ && !shut_down_ && default_service_getter_) {
    default_service_ = default_service_getter_.Run();
    if (default_service_) {
      default_service_->AddObserver(
          const_cast<SharedSubscriptionServiceDecorator*>(this));
    } else {
      LOG(WARNING) << "[eyeo] Default profile SubscriptionService not "
                      "available, using profile-local snapshot only";
    }
  }
  return default_service_;
}

void SharedSubscriptionServiceDecorator::StopObserving() {
  if (shut_down_) {
    return;
  }
  shut_down_ = true;
  if (default_service_) {
    default_service_->RemoveObserver(this);
    default_service_ = nullptr;
  }
  wrapped_->RemoveObserver(this);
}

}  // namespace adblock
