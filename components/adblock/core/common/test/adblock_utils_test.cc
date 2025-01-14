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

#include "components/adblock/core/common/adblock_utils.h"

#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {
namespace utils {

TEST(AdblockUtils, CreateAllowlistFilterForValidInput) {
  EXPECT_EQ(CreateDomainAllowlistingFilter("google.com"),
            "@@||google.com^$document,domain=google.com");
  EXPECT_EQ(CreateDomainAllowlistingFilter("GooGle.com"),
            "@@||google.com^$document,domain=google.com");
}

TEST(AdblockUtils, BaseTimeConverterToVersion) {
  base::Time date;
  EXPECT_TRUE(base::Time::FromString("Thu, 1 Apr 2021 00:01:01 GMT", &date));
  EXPECT_EQ(utils::ConvertBaseTimeToABPFilterVersionFormat(date),
            "202104010001");
}

}  // namespace utils
}  // namespace adblock
