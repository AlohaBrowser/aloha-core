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

// ALOHA https://app.clickup.com/t/2558578/86eyfccqg: tests for the non-default-profile subscription
// service decorator (downloaded lists shared from the Default profile).

#include "components/adblock/content/browser/shared_subscription_service_decorator.h"

#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/configuration/test/fake_filtering_configuration.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {

constexpr char kUserCustomConfigName[] = "user_custom_incognito";
constexpr char kDefaultUserCustomConfigName[] = "user_custom";

std::unique_ptr<SubscriptionCollection> MakeCollection(
    const std::string& configuration_name) {
  auto collection = std::make_unique<MockSubscriptionCollection>();
  ON_CALL(*collection, GetFilteringConfigurationName())
      .WillByDefault(testing::ReturnRefOfCopy(configuration_name));
  return collection;
}

std::vector<std::string> CollectionNames(
    const SubscriptionService::Snapshot& snapshot) {
  std::vector<std::string> names;
  for (const auto& collection : snapshot) {
    names.push_back(collection->GetFilteringConfigurationName());
  }
  return names;
}

class CountingObserver : public SubscriptionService::SubscriptionObserver {
 public:
  void OnSubscriptionInstalled(const GURL& subscription_url) override {
    installed_urls.push_back(subscription_url);
  }
  void OnFilteringConfigurationInstalled(
      FilteringConfiguration* config) override {
    installed_configs.push_back(config->GetName());
  }
  void OnFilteringConfigurationUninstalled(
      std::string_view config_name) override {
    uninstalled_configs.emplace_back(config_name);
  }

  std::vector<GURL> installed_urls;
  std::vector<std::string> installed_configs;
  std::vector<std::string> uninstalled_configs;
};

}  // namespace

class SharedSubscriptionServiceDecoratorTest : public testing::Test {
 public:
  void CreateDecorator() {
    auto wrapped = std::make_unique<MockSubscriptionService>();
    wrapped_ = wrapped.get();
    ON_CALL(*wrapped_, GetCurrentSnapshot()).WillByDefault([]() {
      SubscriptionService::Snapshot snapshot;
      snapshot.push_back(MakeCollection(kUserCustomConfigName));
      return snapshot;
    });
    decorator_ = std::make_unique<SharedSubscriptionServiceDecorator>(
        std::move(wrapped),
        base::BindRepeating(
            &SharedSubscriptionServiceDecoratorTest::GetDefaultService,
            base::Unretained(this)));
  }

  void MakeDefaultServiceAvailable() {
    default_service_available_ = true;
    ON_CALL(default_service_, GetCurrentSnapshot()).WillByDefault([]() {
      SubscriptionService::Snapshot snapshot;
      snapshot.push_back(MakeCollection(kAdblockFilteringConfigurationName));
      snapshot.push_back(MakeCollection(kDefaultUserCustomConfigName));
      return snapshot;
    });
  }

  SubscriptionService* GetDefaultService() {
    return default_service_available_ ? &default_service_ : nullptr;
  }

 protected:
  bool default_service_available_ = false;
  MockSubscriptionService default_service_;
  raw_ptr<MockSubscriptionService> wrapped_ = nullptr;
  std::unique_ptr<SharedSubscriptionServiceDecorator> decorator_;
};

TEST_F(SharedSubscriptionServiceDecoratorTest,
       SnapshotMergesEntireDefaultSnapshot) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  const auto snapshot = decorator_->GetCurrentSnapshot();
  // Own collections + ALL of Default's collections: filtering in a
  // non-default profile matches the Default profile exactly.
  EXPECT_THAT(CollectionNames(snapshot),
              testing::UnorderedElementsAre(
                  kUserCustomConfigName, kAdblockFilteringConfigurationName,
                  kDefaultUserCustomConfigName));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       SnapshotDegradesToWrappedOnlyWithoutDefaultService) {
  CreateDecorator();

  const auto snapshot = decorator_->GetCurrentSnapshot();
  EXPECT_THAT(CollectionNames(snapshot),
              testing::UnorderedElementsAre(kUserCustomConfigName));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       SnapshotPicksUpDefaultServiceAppearingLater) {
  CreateDecorator();
  EXPECT_THAT(CollectionNames(decorator_->GetCurrentSnapshot()),
              testing::UnorderedElementsAre(kUserCustomConfigName));

  // The Default profile's service becomes available afterwards (e.g. the
  // non-default profile was created first). Subsequent snapshots must
  // include Default's collections without recreating the service.
  MakeDefaultServiceAvailable();
  EXPECT_THAT(CollectionNames(decorator_->GetCurrentSnapshot()),
              testing::UnorderedElementsAre(
                  kUserCustomConfigName, kAdblockFilteringConfigurationName,
                  kDefaultUserCustomConfigName));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       DefaultSubscriptionInstallRebroadcastToOwnObserversOnce) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  CountingObserver observer;
  decorator_->AddObserver(&observer);

  // The decorator registered itself as the observer of the Default service.
  ASSERT_TRUE(default_service_.observer_);
  const GURL subscription_url{"https://easylist.example.com/easylist.txt"};
  default_service_.observer_->OnSubscriptionInstalled(subscription_url);

  EXPECT_THAT(observer.installed_urls,
              testing::ElementsAre(subscription_url));
  decorator_->RemoveObserver(&observer);
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       WrappedEventsRebroadcastToOwnObservers) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  CountingObserver observer;
  decorator_->AddObserver(&observer);

  ASSERT_TRUE(wrapped_->observer_);
  FakeFilteringConfiguration config(kUserCustomConfigName);
  wrapped_->observer_->OnFilteringConfigurationInstalled(&config);
  wrapped_->observer_->OnFilteringConfigurationUninstalled(
      kUserCustomConfigName);

  EXPECT_THAT(observer.installed_configs,
              testing::ElementsAre(kUserCustomConfigName));
  EXPECT_THAT(observer.uninstalled_configs,
              testing::ElementsAre(kUserCustomConfigName));
  decorator_->RemoveObserver(&observer);
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       InstallForwardsToWrappedAndStripsFilterLists) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  std::unique_ptr<FilteringConfiguration> installed;
  EXPECT_CALL(*wrapped_, InstallFilteringConfiguration(testing::_))
      .WillOnce([&](std::unique_ptr<FilteringConfiguration> configuration) {
        installed = std::move(configuration);
      });
  EXPECT_CALL(default_service_, InstallFilteringConfiguration(testing::_))
      .Times(0);

  auto config = std::make_unique<FakeFilteringConfiguration>(
      std::string(kAdblockFilteringConfigurationName) + "_incognito");
  config->AddFilterList(GURL{"https://easylist.example.com/easylist.txt"});
  config->AddAllowedDomain("example.com");
  config->AddCustomFilter("||ads.example.com^");
  auto* config_ptr = config.get();

  decorator_->InstallFilteringConfiguration(std::move(config));

  // The configuration object survives (JNI callers keep raw pointers to it),
  // reaches only the wrapped service, and loses its downloading filter
  // lists while custom rules stay intact.
  ASSERT_TRUE(installed);
  EXPECT_EQ(installed.get(), config_ptr);
  EXPECT_TRUE(installed->GetFilterLists().empty());
  EXPECT_THAT(installed->GetAllowedDomains(),
              testing::ElementsAre("example.com"));
  EXPECT_THAT(installed->GetCustomFilters(),
              testing::ElementsAre("||ads.example.com^"));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       QueriesAndMutationsForwardToWrappedOnly) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  EXPECT_CALL(*wrapped_, UninstallFilteringConfiguration(
                             std::string_view(kUserCustomConfigName)));
  EXPECT_CALL(default_service_, UninstallFilteringConfiguration(testing::_))
      .Times(0);
  decorator_->UninstallFilteringConfiguration(kUserCustomConfigName);

  EXPECT_CALL(*wrapped_,
              GetFilteringConfiguration(
                  std::string_view(kUserCustomConfigName)));
  decorator_->GetFilteringConfiguration(kUserCustomConfigName);
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       InstalledConfigurationsMergeDefaults) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  FakeFilteringConfiguration own_config(kUserCustomConfigName);
  FakeFilteringConfiguration default_config(
      kAdblockFilteringConfigurationName);
  EXPECT_CALL(*wrapped_, GetInstalledFilteringConfigurations())
      .WillOnce(testing::Return(
          std::vector<FilteringConfiguration*>{&own_config}));
  EXPECT_CALL(default_service_, GetInstalledFilteringConfigurations())
      .WillOnce(testing::Return(
          std::vector<FilteringConfiguration*>{&default_config}));

  // Default's configurations are visible through the decorator, so
  // IsFilteringNeeded()-style gates open with no per-profile setup.
  EXPECT_THAT(decorator_->GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(&own_config, &default_config));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       InstalledConfigurationsWithoutDefaultService) {
  CreateDecorator();

  FakeFilteringConfiguration own_config(kUserCustomConfigName);
  EXPECT_CALL(*wrapped_, GetInstalledFilteringConfigurations())
      .WillOnce(testing::Return(
          std::vector<FilteringConfiguration*>{&own_config}));

  EXPECT_THAT(decorator_->GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(&own_config));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       InstalledConfigurationsDedupedByName) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  // On a name collision (a stale local config persisted by an older
  // version) Default's configuration wins, consistently with the snapshot
  // merge and name lookups - otherwise a disabled local stub could shadow
  // Default's enabled config and close the IsFilteringNeeded gate.
  FakeFilteringConfiguration own_config(kAdblockFilteringConfigurationName);
  FakeFilteringConfiguration default_config(
      kAdblockFilteringConfigurationName);
  ON_CALL(*wrapped_, GetInstalledFilteringConfigurations())
      .WillByDefault(testing::Return(
          std::vector<FilteringConfiguration*>{&own_config}));
  ON_CALL(default_service_, GetInstalledFilteringConfigurations())
      .WillByDefault(testing::Return(
          std::vector<FilteringConfiguration*>{&default_config}));

  EXPECT_THAT(decorator_->GetInstalledFilteringConfigurations(),
              testing::UnorderedElementsAre(&default_config));
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       GetFilteringConfigurationPrefersDefaultOnCollision) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  FakeFilteringConfiguration own_config(kAdblockFilteringConfigurationName);
  FakeFilteringConfiguration default_config(
      kAdblockFilteringConfigurationName);
  ON_CALL(*wrapped_,
          GetFilteringConfiguration(
              std::string_view(kAdblockFilteringConfigurationName)))
      .WillByDefault(testing::Return(&own_config));
  ON_CALL(default_service_,
          GetFilteringConfiguration(
              std::string_view(kAdblockFilteringConfigurationName)))
      .WillByDefault(testing::Return(&default_config));

  EXPECT_EQ(decorator_->GetFilteringConfiguration(
                kAdblockFilteringConfigurationName),
            &default_config);
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       GetFilteringConfigurationFallsBackToDefault) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  FakeFilteringConfiguration default_config(
      kAdblockFilteringConfigurationName);
  ON_CALL(*wrapped_, GetFilteringConfiguration(testing::_))
      .WillByDefault(testing::Return(nullptr));
  ON_CALL(default_service_,
          GetFilteringConfiguration(
              std::string_view(kAdblockFilteringConfigurationName)))
      .WillByDefault(testing::Return(&default_config));

  // A local miss resolves to Default's configuration instead of nullptr, so
  // name-based callers (JNI) reuse it rather than inventing an empty local
  // configuration.
  EXPECT_EQ(decorator_->GetFilteringConfiguration(
                kAdblockFilteringConfigurationName),
            &default_config);
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       GetCurrentSubscriptionsRoutesToOwningService) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  FakeFilteringConfiguration own_config(kUserCustomConfigName);
  FakeFilteringConfiguration default_config(
      kAdblockFilteringConfigurationName);
  FakeFilteringConfiguration unknown_config("unknown");
  ON_CALL(*wrapped_, GetInstalledFilteringConfigurations())
      .WillByDefault(testing::Return(
          std::vector<FilteringConfiguration*>{&own_config}));
  ON_CALL(default_service_, GetInstalledFilteringConfigurations())
      .WillByDefault(testing::Return(
          std::vector<FilteringConfiguration*>{&default_config}));

  EXPECT_CALL(*wrapped_, GetCurrentSubscriptions(&own_config));
  decorator_->GetCurrentSubscriptions(&own_config);

  // Default's configuration (handed out by the merged list) must be routed
  // to the Default service - passing it into the wrapped SubscriptionService
  // impl would be undefined behavior.
  EXPECT_CALL(*wrapped_, GetCurrentSubscriptions(&default_config)).Times(0);
  EXPECT_CALL(default_service_, GetCurrentSubscriptions(&default_config));
  decorator_->GetCurrentSubscriptions(&default_config);

  EXPECT_CALL(*wrapped_, GetCurrentSubscriptions(&unknown_config)).Times(0);
  EXPECT_CALL(default_service_, GetCurrentSubscriptions(&unknown_config))
      .Times(0);
  EXPECT_TRUE(decorator_->GetCurrentSubscriptions(&unknown_config).empty());
}

TEST_F(SharedSubscriptionServiceDecoratorTest,
       SnapshotNameCollisionPrefersDefaultCollection) {
  MakeDefaultServiceAvailable();
  CreateDecorator();

  // Own snapshot contains a stale near-empty "adblock" collection; the
  // merged snapshot must keep exactly one "adblock" entry (Default's), or
  // document-allowlisting would break (it requires an allowing rule from
  // every collection).
  ON_CALL(*wrapped_, GetCurrentSnapshot()).WillByDefault([]() {
    SubscriptionService::Snapshot snapshot;
    snapshot.push_back(MakeCollection(kUserCustomConfigName));
    snapshot.push_back(MakeCollection(kAdblockFilteringConfigurationName));
    return snapshot;
  });

  EXPECT_THAT(CollectionNames(decorator_->GetCurrentSnapshot()),
              testing::UnorderedElementsAre(
                  kUserCustomConfigName, kAdblockFilteringConfigurationName,
                  kDefaultUserCustomConfigName));
}

}  // namespace adblock
