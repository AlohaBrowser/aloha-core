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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_SHARED_SUBSCRIPTION_SERVICE_DECORATOR_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_SHARED_SUBSCRIPTION_SERVICE_DECORATOR_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/subscription/subscription_service.h"

namespace adblock {

// ALOHA https://app.clickup.com/t/2558578/86eyfccqg: SubscriptionService for non-default profiles.
//
// Filter lists are downloaded, stored and indexed exclusively by the Default
// profile's SubscriptionService. Non-default profiles (e.g. Incognito) get
// this decorator instead of a plain SubscriptionServiceImpl:
// - GetCurrentSnapshot() merges the Default profile's entire snapshot (both
//   downloaded lists and its user_custom rules) into the wrapped
//   (per-profile) snapshot, live per call. Filtering in a non-default
//   profile is therefore identical to the Default profile without any
//   embedder-side (Kotlin) setup of per-profile configurations.
// - GetInstalledFilteringConfigurations() also includes the Default
//   profile's configurations, so IsFilteringNeeded()-style gates open even
//   when the non-default profile has no configurations of its own.
// - All other read APIs are consistent with that merged view, with a single
//   collision rule: on a name collision Default's configuration/collection
//   wins everywhere (list, name lookup, download stats, snapshot); local
//   configurations resolve when Default has no configuration of that name.
//   GetCurrentSubscriptions() routes by the owning service of the exact
//   configuration pointer. Mutations (install/uninstall of configurations)
//   act on the wrapped per-profile service only.
// - InstallFilteringConfiguration() strips filter lists from installed
//   configurations, guaranteeing the wrapped service never downloads or
//   stores subscriptions on disk.
// - Events from both the wrapped and the Default service are re-broadcast
//   to this decorator's own observers, so consumers of the non-default
//   profile react when Default finishes downloading its lists.
//
// Both services live on the UI thread; SubscriptionCollections are cheap,
// immutable, thread-safe handles, so merging Default's collections into a
// snapshot consumed by another profile's classification is safe.
class SharedSubscriptionServiceDecorator
    : public SubscriptionService,
      public SubscriptionService::SubscriptionObserver {
 public:
  using DefaultServiceGetter = base::RepeatingCallback<SubscriptionService*()>;

  // |wrapped| is the non-default profile's own service (carries only
  // user_custom-style configurations). |default_service_getter| resolves the
  // Default profile's SubscriptionService; it may return nullptr, in which
  // case the decorator degrades to wrapped-only behavior.
  SharedSubscriptionServiceDecorator(
      std::unique_ptr<SubscriptionService> wrapped,
      DefaultServiceGetter default_service_getter);
  ~SharedSubscriptionServiceDecorator() override;

  SharedSubscriptionServiceDecorator(const SharedSubscriptionServiceDecorator&) =
      delete;
  SharedSubscriptionServiceDecorator& operator=(
      const SharedSubscriptionServiceDecorator&) = delete;

  // SubscriptionService:
  std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions(
      FilteringConfiguration* configuration) const override;
  void InstallFilteringConfiguration(
      std::unique_ptr<FilteringConfiguration> configuration) override;
  void UninstallFilteringConfiguration(
      std::string_view configuration_name) override;
  std::vector<FilteringConfiguration*> GetInstalledFilteringConfigurations()
      override;
  FilteringConfiguration* GetFilteringConfiguration(
      std::string_view configuration_name) const override;
  Snapshot GetCurrentSnapshot() const override;
  std::map<GURL, utils::DownloadStats> GetDownloadStats(
      std::string_view configuration_name) const override;
  void SetAutoInstallEnabled(bool enabled) override;
  bool IsAutoInstallEnabled() const override;
  void AddObserver(SubscriptionObserver* observer) override;
  void RemoveObserver(SubscriptionObserver* observer) override;

  // KeyedService:
  void Shutdown() override;

  // SubscriptionService::SubscriptionObserver (events arriving from either
  // the wrapped or the Default service, re-broadcast to |observers_|):
  void OnSubscriptionInstalled(const GURL& subscription_url) override;
  void OnFilteringConfigurationInstalled(
      FilteringConfiguration* config) override;
  void OnFilteringConfigurationUninstalled(
      std::string_view config_name) override;

 private:
  // Resolves (and starts observing) the Default service on first success.
  // Returns nullptr if the getter cannot resolve it (yet).
  SubscriptionService* GetDefaultService() const;
  void StopObserving();

  SEQUENCE_CHECKER(sequence_checker_);
  std::unique_ptr<SubscriptionService> wrapped_;
  DefaultServiceGetter default_service_getter_;
  // Lazily resolved and cached. The Default profile outlives non-default
  // profiles (it cannot be deleted from the context store), so this pointer
  // stays valid for the decorator's lifetime once resolved.
  mutable raw_ptr<SubscriptionService> default_service_ = nullptr;
  bool shut_down_ = false;
  base::ObserverList<SubscriptionObserver> observers_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_SHARED_SUBSCRIPTION_SERVICE_DECORATOR_H_
