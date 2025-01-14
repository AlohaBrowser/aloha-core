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

#ifndef COMPONENTS_ADBLOCK_CORE_NET_TEST_MOCK_ADBLOCK_RESOURCE_REQUEST_H_
#define COMPONENTS_ADBLOCK_CORE_NET_TEST_MOCK_ADBLOCK_RESOURCE_REQUEST_H_

#include "components/adblock/core/net/adblock_resource_request.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::NiceMock;

namespace adblock {

class MockAdblockResourceRequest : public NiceMock<AdblockResourceRequest> {
 public:
  MockAdblockResourceRequest();
  ~MockAdblockResourceRequest() override;
  MOCK_METHOD(void,
              Start,
              (GURL url,
               Method method,
               ResponseCallback response_callback,
               RetryPolicy retry_policy,
               const std::string extra_query_params),
              (override));
  MOCK_METHOD(void,
              Redirect,
              (GURL redirect_url, const std::string extra_query_params),
              (override));
  MOCK_METHOD(size_t, GetNumberOfRedirects, (), (const, override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_NET_TEST_MOCK_ADBLOCK_RESOURCE_REQUEST_H_
