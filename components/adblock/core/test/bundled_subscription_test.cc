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

#include <memory>

#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

TEST(AdblockBundledSubscriptionTest, EasyListIsValid) {
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  flatbuffers::Verifier verifier(buffer->data(), buffer->size());
  EXPECT_TRUE(flat::VerifySubscriptionBuffer(verifier));
}

TEST(AdblockBundledSubscriptionTest, ExceptionrulesIsValid) {
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES);
  flatbuffers::Verifier verifier(buffer->data(), buffer->size());
  EXPECT_TRUE(flat::VerifySubscriptionBuffer(verifier));
}

TEST(AdblockBundledSubscriptionTest, AnticvIsValid) {
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_ANTICV);
  flatbuffers::Verifier verifier(buffer->data(), buffer->size());
  EXPECT_TRUE(flat::VerifySubscriptionBuffer(verifier));
}

}  // namespace adblock
