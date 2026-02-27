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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_PERSISTENT_METADATA_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_PERSISTENT_METADATA_H_

#include "components/adblock/core/subscription/subscription_persistent_metadata.h"

#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockSubscriptionPersistentMetadata
    : public NiceMock<SubscriptionPersistentMetadata> {
 public:
  MockSubscriptionPersistentMetadata();
  ~MockSubscriptionPersistentMetadata() override;
  MOCK_METHOD(void,
              SetExpirationInterval,
              (const GURL& subscription_url, base::TimeDelta expires_in),
              (override));
  MOCK_METHOD(void,
              SetLastInstallationTime,
              (const GURL& subscription_url),
              (override));
  MOCK_METHOD(void,
              SetVersion,
              (const GURL& subscription_url, std::string version),
              (override));
  MOCK_METHOD(void,
              IncrementDownloadSuccessCount,
              (const GURL& subscription_url),
              (override));
  MOCK_METHOD(void,
              IncrementDownloadErrorCount,
              (const GURL& subscription_url),
              (override));
  MOCK_METHOD(bool,
              IsExpired,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(base::Time,
              GetLastInstallationTime,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(std::string,
              GetVersion,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(int,
              GetDownloadSuccessCount,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(int,
              GetDownloadErrorCount,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(void,
              SetAutoInstalledExpirationInterval,
              (const GURL& subscription_url, base::TimeDelta expires_in),
              (override));
  MOCK_METHOD(bool,
              IsAutoInstalled,
              (const GURL& subscription_url),
              (override, const));
  MOCK_METHOD(bool,
              IsAutoInstalledExpired,
              (const GURL& subscription_url),
              (override, const));

  MOCK_METHOD(void, RemoveMetadata, (const GURL& subscription_url), (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_PERSISTENT_METADATA_H_
