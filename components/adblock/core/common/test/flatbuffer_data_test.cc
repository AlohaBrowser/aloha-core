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

#include "components/adblock/core/common/flatbuffer_data.h"

#include <memory>
#include <string_view>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockMemoryMappedFlatbufferDataTest : public testing::Test {
 public:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(AdblockMemoryMappedFlatbufferDataTest, FileContentIsReadLikeMemory) {
  base::FilePath test_file = temp_dir_.GetPath().AppendASCII("data.fb");
  ASSERT_TRUE(base::WriteFile(test_file, "content"));

  auto buffer = std::make_unique<MemoryMappedFlatbufferData>(test_file);
  auto span = std::string_view(reinterpret_cast<const char*>(buffer->data()),
                               buffer->size());
  EXPECT_EQ(span, "content");
}

TEST_F(AdblockMemoryMappedFlatbufferDataTest,
       PermanentlyRemoveSourceOnDestruction) {
  base::FilePath test_file = temp_dir_.GetPath().AppendASCII("data.fb");
  ASSERT_TRUE(base::WriteFile(test_file, "content"));

  auto buffer = std::make_unique<MemoryMappedFlatbufferData>(test_file);
  buffer->PermanentlyRemoveSourceOnDestruction();

  // File still present since buffer is alive.
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(base::PathExists(test_file));

  // Buffer dies, destroys file.
  buffer.reset();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(base::PathExists(test_file));
}

TEST_F(AdblockMemoryMappedFlatbufferDataTest, SourceNotDestroyedWhenNotAsked) {
  base::FilePath test_file = temp_dir_.GetPath().AppendASCII("data.fb");
  ASSERT_TRUE(base::WriteFile(test_file, "content"));

  auto buffer = std::make_unique<MemoryMappedFlatbufferData>(test_file);

  // Buffer dies, source remains on disk as
  // PermanentlyRemoveSourceOnDestruction() was not called.
  buffer.reset();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(base::PathExists(test_file));
}

}  // namespace adblock
