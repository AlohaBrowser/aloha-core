
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

#include "components/adblock/core/configuration/persistent_filtering_configuration.h"

#include <memory>

#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace adblock {

namespace {

class MockObserver : public FilteringConfiguration::Observer {
 public:
  MOCK_METHOD(void,
              OnEnabledStateChanged,
              (FilteringConfiguration * config),
              (override));
  MOCK_METHOD(void,
              OnFilterListsChanged,
              (FilteringConfiguration * config),
              (override));
  MOCK_METHOD(void,
              OnAllowedDomainsChanged,
              (FilteringConfiguration * config),
              (override));
  MOCK_METHOD(void,
              OnCustomFiltersChanged,
              (FilteringConfiguration * config),
              (override));
};
}  // namespace

// Wether the testee is destroyed and recreated by MaybeRecreateConfiguration().
// Recreating validates persistence over browser restarts while not recreating
// validates behavior within a single session.
enum class RecreateConfiguration {
  Yes,
  No,
};

class AdblockPersistentFilteringConfigurationTest
    : public testing::TestWithParam<RecreateConfiguration> {
 public:
  void SetUp() override {
    adblock::common::prefs::RegisterProfilePrefs(pref_service_.registry());
    RecreateConfiguration();
  }

  void TearDown() override { configuration_->RemoveObserver(&observer_); }

  void RecreateConfiguration() {
    if (configuration_) {
      testing::Mock::VerifyAndClearExpectations(&observer_);
      configuration_->RemoveObserver(&observer_);
    }
    configuration_ = std::make_unique<PersistentFilteringConfiguration>(
        &pref_service_, kName);
    configuration_->AddObserver(&observer_);
  }

  void MaybeRecreateConfiguration() {
    if (GetParam() == RecreateConfiguration::Yes) {
      RecreateConfiguration();
    }
  }

  const std::string kName = "adblock";
  const GURL kUrl1{"https://list.com/filters1.txt"};
  const GURL kUrl2{"https://list.com/filters2.txt"};
  const GURL kUrl3{"https://list.com/filters3.txt"};
  const std::string kAllowedDomain1{"www.domain1.com"};
  const std::string kAllowedDomain2{"www.domain2.com"};
  const std::string kAllowedDomain3{"www.domain3.com"};
  const std::string kCustomFilter1{"@@^domain1.com"};
  const std::string kCustomFilter2{"@@^domain2.com"};
  const std::string kCustomFilter3{"@@^domain3.com"};
  MockObserver observer_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<PersistentFilteringConfiguration> configuration_;
};

TEST_P(AdblockPersistentFilteringConfigurationTest, NameStored) {
  MaybeRecreateConfiguration();
  EXPECT_EQ(configuration_->GetName(), kName);
}

TEST_P(AdblockPersistentFilteringConfigurationTest, EnabledStateStored) {
  // No notification for setting Enabled to true because it is the default
  // state.
  EXPECT_CALL(observer_, OnEnabledStateChanged(configuration_.get())).Times(0);
  configuration_->SetEnabled(true);
  MaybeRecreateConfiguration();
  EXPECT_TRUE(configuration_->IsEnabled());

  EXPECT_CALL(observer_, OnEnabledStateChanged(configuration_.get()));
  configuration_->SetEnabled(false);
  MaybeRecreateConfiguration();
  EXPECT_FALSE(configuration_->IsEnabled());
}

TEST_P(AdblockPersistentFilteringConfigurationTest, FilterListAdded) {
  // List initially empty.
  EXPECT_TRUE(configuration_->GetFilterLists().empty());
  // Observer will be notified about addition.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get()));
  configuration_->AddFilterList(kUrl1);

  // New URL is returned consistently.
  MaybeRecreateConfiguration();
  EXPECT_TRUE(configuration_->IsFilterListPresent(kUrl1));
  EXPECT_THAT(configuration_->GetFilterLists(),
              testing::UnorderedElementsAre(kUrl1));
}

TEST_P(AdblockPersistentFilteringConfigurationTest, FilterListRemoved) {
  // Observer will be notified about addition.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get()));
  configuration_->AddFilterList(kUrl1);
  // Observer will be notified about removal.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get()));
  configuration_->RemoveFilterList(kUrl1);

  // Removed URL is no longer returned.
  MaybeRecreateConfiguration();
  EXPECT_FALSE(configuration_->IsFilterListPresent(kUrl1));
  EXPECT_TRUE(configuration_->GetFilterLists().empty());
}

TEST_P(AdblockPersistentFilteringConfigurationTest, MultipleFilterLists) {
  // Observer will be notified about all additions.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get())).Times(3);
  configuration_->AddFilterList(kUrl1);
  configuration_->AddFilterList(kUrl2);
  configuration_->AddFilterList(kUrl3);
  EXPECT_TRUE(configuration_->IsFilterListPresent(kUrl1));
  EXPECT_TRUE(configuration_->IsFilterListPresent(kUrl2));
  EXPECT_TRUE(configuration_->IsFilterListPresent(kUrl3));
  // Observer will be notified about one removal.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get()));
  configuration_->RemoveFilterList(kUrl2);
  EXPECT_FALSE(configuration_->IsFilterListPresent(kUrl2));

  // Remaining lists are returned.
  MaybeRecreateConfiguration();
  EXPECT_THAT(configuration_->GetFilterLists(),
              testing::UnorderedElementsAre(kUrl1, kUrl3));
}

TEST_P(AdblockPersistentFilteringConfigurationTest,
       DuplicateFilterListsIgnored) {
  // Observer will be notified about only one addition.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get())).Times(1);
  configuration_->AddFilterList(kUrl1);
  configuration_->AddFilterList(kUrl1);
  configuration_->AddFilterList(kUrl1);

  // Duplicate URL was ignored, only one instance returned.
  MaybeRecreateConfiguration();
  EXPECT_THAT(configuration_->GetFilterLists(),
              testing::UnorderedElementsAre(kUrl1));
}

TEST_P(AdblockPersistentFilteringConfigurationTest,
       SpuriousFilterListRemovalIgnored) {
  // Observer will be notified about one addition.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get())).Times(1);
  configuration_->AddFilterList(kUrl1);
  // Observer will be notified about one removal.
  EXPECT_CALL(observer_, OnFilterListsChanged(configuration_.get())).Times(1);
  configuration_->RemoveFilterList(kUrl1);
  configuration_->RemoveFilterList(kUrl1);
  configuration_->RemoveFilterList(kUrl1);

  MaybeRecreateConfiguration();
  EXPECT_TRUE(configuration_->GetFilterLists().empty());
}

TEST_P(AdblockPersistentFilteringConfigurationTest, MultipleAllowedDomains) {
  // List initially empty.
  EXPECT_TRUE(configuration_->GetAllowedDomains().empty());
  // Add some allowed domains.
  EXPECT_CALL(observer_, OnAllowedDomainsChanged(configuration_.get()))
      .Times(3);
  configuration_->AddAllowedDomain(kAllowedDomain1);
  configuration_->AddAllowedDomain(kAllowedDomain2);
  configuration_->AddAllowedDomain(kAllowedDomain3);
  // Spurious addition:
  configuration_->AddAllowedDomain(kAllowedDomain3);
  EXPECT_CALL(observer_, OnAllowedDomainsChanged(configuration_.get()))
      .Times(1);
  configuration_->RemoveAllowedDomain(kAllowedDomain2);
  // Spurious removal:
  configuration_->RemoveAllowedDomain(kAllowedDomain2);

  MaybeRecreateConfiguration();
  EXPECT_THAT(configuration_->GetAllowedDomains(),
              testing::UnorderedElementsAre(kAllowedDomain1, kAllowedDomain3));
}

TEST_P(AdblockPersistentFilteringConfigurationTest, MultipleCustomFilters) {
  // List initially empty.
  EXPECT_TRUE(configuration_->GetCustomFilters().empty());
  // Add some custom filters.
  EXPECT_CALL(observer_, OnCustomFiltersChanged(configuration_.get())).Times(3);
  configuration_->AddCustomFilter(kCustomFilter1);
  configuration_->AddCustomFilter(kCustomFilter2);
  configuration_->AddCustomFilter(kCustomFilter3);
  // Spurious addition:
  configuration_->AddCustomFilter(kCustomFilter3);

  EXPECT_CALL(observer_, OnCustomFiltersChanged(configuration_.get())).Times(1);
  configuration_->RemoveCustomFilter(kCustomFilter2);
  // Spurious removal:
  configuration_->RemoveCustomFilter(kCustomFilter2);

  MaybeRecreateConfiguration();
  EXPECT_THAT(configuration_->GetCustomFilters(),
              testing::UnorderedElementsAre(kCustomFilter1, kCustomFilter3));
}

TEST_F(AdblockPersistentFilteringConfigurationTest,
       PrefsClearedAfterRemovePersistedData) {
  const auto& all_configurations =
      pref_service_.GetValue(common::prefs::kConfigurationsPrefsPath).GetDict();
  EXPECT_NE(nullptr, all_configurations.FindDict(kName));
  PersistentFilteringConfiguration::RemovePersistedData(&pref_service_, kName);
  EXPECT_EQ(nullptr, all_configurations.FindDict(kName));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockPersistentFilteringConfigurationTest,
                         testing::Values(RecreateConfiguration::Yes,
                                         RecreateConfiguration::No));

}  // namespace adblock
