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

#include "components/adblock/core/subscription/subscription_persistent_metadata_impl.h"

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

// We check two scenarios - whether testee retains relevant values as it is
// used, and whether it persists them into Prefs.
enum class PersistenceType { ValuesPersistInMemory, ValuesPersistInPrefs };

class AdblockSubscriptionPersistentMetadataImplTest
    : public testing::TestWithParam<PersistenceType> {
 public:
  AdblockSubscriptionPersistentMetadataImplTest() {
    common::prefs::RegisterProfilePrefs(prefs_.registry());
    metadata_ = std::make_unique<SubscriptionPersistentMetadataImpl>(&prefs_);
  }

  void MaybeRecreateMetadata() {
    if (GetParam() == PersistenceType::ValuesPersistInPrefs) {
      // Recreate SubscriptionPersistentMetadataImpl to make sure it re-reads
      // its state from prefs.
      metadata_ = std::make_unique<SubscriptionPersistentMetadataImpl>(&prefs_);
    } else {
      // Do nothing, verify SubscriptionPersistentMetadataImpl maintains a
      // consistent state in memory/
    }
  }

 protected:
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<SubscriptionPersistentMetadataImpl> metadata_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  const GURL kUrl1{"https://subscription.com/filters1.txt"};
  const GURL kUrl2{"https://subscription.com/filters2.txt"};
};

TEST_F(AdblockSubscriptionPersistentMetadataImplTest,
       UntrackedSubscriptionConsideredExpired) {
  // Normally, production code should only ask about expiration of installed
  // subscriptions, and subscriptions should always have an expiration date set
  // during installation. If there's any corruption though, consider the
  // subscription expired to trigger an update and, hopefully, force setting a
  // real expiration time.
  EXPECT_TRUE(metadata_->IsExpired(kUrl1));
}

TEST_F(AdblockSubscriptionPersistentMetadataImplTest,
       UnversionedSubscriptionReturnsZeroVersion) {
  // It is valid for a subscription to not have a version set. In that case,
  // we're expected to send "0" in download request's relevant query param.
  EXPECT_EQ("0", metadata_->GetVersion(kUrl1));
}

TEST_F(AdblockSubscriptionPersistentMetadataImplTest,
       UntrackedSubscriptionErrorAndDownloadCountAreZero) {
  // During first-time installation, it is expected that there's no download
  // count or error count registered yet.
  EXPECT_EQ(0, metadata_->GetDownloadSuccessCount(kUrl1));
  EXPECT_EQ(0, metadata_->GetDownloadErrorCount(kUrl1));
}

TEST_F(AdblockSubscriptionPersistentMetadataImplTest,
       UntrackedSubscriptionAutoInstalledExpirationNotExpired) {
  // Since the auto installed expiration time is not mandatory for a
  // subscription and it is used for removing auto installed subscriptions false
  // is returned for not yet installed subscriptions.
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest, ExpirationTimeTracked) {
  const auto expiration1 = base::Days(1);
  const auto expiration2 = base::Days(2);
  metadata_->SetExpirationInterval(kUrl1, expiration1);
  metadata_->SetExpirationInterval(kUrl2, expiration2);

  // Both expiration dates are in the future.
  EXPECT_FALSE(metadata_->IsExpired(kUrl1));
  EXPECT_FALSE(metadata_->IsExpired(kUrl2));

  // Forward clock by 1 day to trigger first expiration.
  task_environment_.AdvanceClock(base::Days(1));

  // First subscription is now expired.
  EXPECT_TRUE(metadata_->IsExpired(kUrl1));
  EXPECT_FALSE(metadata_->IsExpired(kUrl2));

  MaybeRecreateMetadata();

  // Forward clock by another day to trigger second expiration.
  task_environment_.AdvanceClock(base::Days(1));

  // Both subscriptions are now expired.
  EXPECT_TRUE(metadata_->IsExpired(kUrl1));
  EXPECT_TRUE(metadata_->IsExpired(kUrl2));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest,
       LastInstallationTimeTracked) {
  // Last installation time is not yet set.
  EXPECT_EQ(metadata_->GetLastInstallationTime(kUrl1), base::Time());
  metadata_->SetLastInstallationTime(kUrl1);
  // Last installation time gets set.
  EXPECT_EQ(metadata_->GetLastInstallationTime(kUrl1), base::Time::Now());
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest,
       AutoInstalledExpirationTimeTracked) {
  const auto auto_installed_expiration1 = base::Days(1);
  const auto auto_installed_expiration2 = base::Days(2);
  metadata_->SetAutoInstalledExpirationInterval(kUrl1,
                                                auto_installed_expiration1);
  metadata_->SetAutoInstalledExpirationInterval(kUrl2,
                                                auto_installed_expiration2);

  // Both expiration dates are in the future.
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl1));
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl2));

  // Forward clock by 1 day to trigger first expiration.
  task_environment_.AdvanceClock(base::Days(1));

  // First subscription is now expired.
  EXPECT_TRUE(metadata_->IsAutoInstalledExpired(kUrl1));
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl2));

  MaybeRecreateMetadata();

  // Forward clock by another day to trigger second expiration.
  task_environment_.AdvanceClock(base::Days(1));

  // Both subscriptions are now expired.
  EXPECT_TRUE(metadata_->IsAutoInstalledExpired(kUrl1));
  EXPECT_TRUE(metadata_->IsAutoInstalledExpired(kUrl2));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest, VersionTracked) {
  const std::string version1 = "1.0";
  const std::string version2 = "2.0";
  metadata_->SetVersion(kUrl1, version1);
  metadata_->SetVersion(kUrl2, version2);

  MaybeRecreateMetadata();

  EXPECT_EQ(version1, metadata_->GetVersion(kUrl1));
  EXPECT_EQ(version2, metadata_->GetVersion(kUrl2));

  // Versions can be overwritten later.
  const std::string new_version = "3.0";
  metadata_->SetVersion(kUrl1, new_version);

  MaybeRecreateMetadata();

  EXPECT_EQ(new_version, metadata_->GetVersion(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest, DownloadCountTracked) {
  metadata_->IncrementDownloadSuccessCount(kUrl1);
  metadata_->IncrementDownloadSuccessCount(kUrl1);
  metadata_->IncrementDownloadSuccessCount(kUrl1);

  metadata_->IncrementDownloadSuccessCount(kUrl2);

  MaybeRecreateMetadata();

  EXPECT_EQ(3, metadata_->GetDownloadSuccessCount(kUrl1));
  EXPECT_EQ(1, metadata_->GetDownloadSuccessCount(kUrl2));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest, ErrorCountTracked) {
  metadata_->IncrementDownloadErrorCount(kUrl1);
  metadata_->IncrementDownloadErrorCount(kUrl1);
  metadata_->IncrementDownloadErrorCount(kUrl1);

  metadata_->IncrementDownloadErrorCount(kUrl2);

  MaybeRecreateMetadata();

  EXPECT_EQ(3, metadata_->GetDownloadErrorCount(kUrl1));
  EXPECT_EQ(1, metadata_->GetDownloadErrorCount(kUrl2));

  // A successful download resets the error count.
  metadata_->IncrementDownloadSuccessCount(kUrl1);

  MaybeRecreateMetadata();

  EXPECT_EQ(0, metadata_->GetDownloadErrorCount(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest, RemovingMetadata) {
  // Set some values for kUrl1
  metadata_->IncrementDownloadSuccessCount(kUrl1);
  metadata_->IncrementDownloadErrorCount(kUrl1);
  metadata_->SetVersion(kUrl1, "version");
  metadata_->SetExpirationInterval(kUrl1, base::Days(1));
  metadata_->SetAutoInstalledExpirationInterval(kUrl1, base::Days(7));

  // Also set a value for kUrl2
  metadata_->IncrementDownloadErrorCount(kUrl2);
  metadata_->SetAutoInstalledExpirationInterval(kUrl2, base::Days(7));

  metadata_->RemoveMetadata(kUrl1);

  MaybeRecreateMetadata();

  // The value set for kUrl2 is left untouched.
  EXPECT_EQ(1, metadata_->GetDownloadErrorCount(kUrl2));
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl2));

  // The values for kUrl1 are back to defaults.
  EXPECT_TRUE(metadata_->IsExpired(kUrl1));
  EXPECT_EQ("0", metadata_->GetVersion(kUrl1));
  EXPECT_EQ(0, metadata_->GetDownloadSuccessCount(kUrl1));
  EXPECT_EQ(0, metadata_->GetDownloadErrorCount(kUrl1));
  EXPECT_FALSE(metadata_->IsAutoInstalledExpired(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest,
       UntrackedSubscriptionIsNotAutoInstalled) {
  EXPECT_FALSE(metadata_->IsAutoInstalled(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest,
       TrackedSubscriptionIsNotAutoInstalled) {
  const auto expiration1 = base::Days(1);
  metadata_->SetExpirationInterval(kUrl2, expiration1);
  EXPECT_FALSE(metadata_->IsAutoInstalled(kUrl1));
}

TEST_P(AdblockSubscriptionPersistentMetadataImplTest,
       AutoInstalledSubsriptionIsAutoInstalled) {
  const auto auto_installed_expiration1 = base::Days(1);
  metadata_->SetAutoInstalledExpirationInterval(kUrl1,
                                                auto_installed_expiration1);
  EXPECT_TRUE(metadata_->IsAutoInstalled(kUrl1));
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockSubscriptionPersistentMetadataImplTest,
    testing::Values(PersistenceType::ValuesPersistInMemory,
                    PersistenceType::ValuesPersistInPrefs));

}  // namespace adblock
