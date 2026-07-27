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

#include "components/adblock/core/snippet/snippet_storage_impl.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/mock_resource_bundle_delegate.h"
#include "ui/base/resource/resource_bundle.h"

namespace adblock {

class AdblockSnippetStorageImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", &mock_delegate_,
        ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
    EXPECT_CALL(mock_delegate_, LoadDataResourceString(IDR_ADBLOCK_SNIPPETS_JS))
        .WillRepeatedly(testing::Return("snippet_lib"));
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  void TearDown() override { ui::ResourceBundle::CleanupSharedInstance(); }

  base::FilePath GetFileName() {
    return temp_dir_.GetPath().AppendASCII("snippet.js");
  }

  base::test::TaskEnvironment task_environment_;
  ui::ResourceBundle::SharedInstanceSwapperForTesting instance_swapper_;
  testing::NiceMock<ui::MockResourceBundleDelegate> mock_delegate_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(AdblockSnippetStorageImplTest, LoadFromResourceByDefault) {
  SnippetStorageImpl storage(temp_dir_.GetPath());
  storage.LoadSnippets("");
  task_environment_.RunUntilIdle();
  EXPECT_EQ("snippet_lib", storage.GetLoadedSnippets());
}

TEST_F(AdblockSnippetStorageImplTest, LoadFromFileIfAny) {
  SnippetStorageImpl storage(temp_dir_.GetPath());
  std::string data = "LoadFromFileIfAny";
  base::WriteFile(GetFileName(), data);
  storage.LoadSnippets("Fb5eKwJQ2EBIa7Tb+07cYpmpEpjDoTQ0kTeWobVGrT0=");
  task_environment_.RunUntilIdle();
  EXPECT_EQ(data, storage.GetLoadedSnippets());
}

TEST_F(AdblockSnippetStorageImplTest, LoadFromFileInvalidSignature) {
  SnippetStorageImpl storage(temp_dir_.GetPath());
  storage.LoadSnippets("xxx");
  task_environment_.RunUntilIdle();
  std::string data = "LoadFromFileInvalidSignature";
  base::WriteFile(GetFileName(), data);

  EXPECT_EQ("snippet_lib", storage.GetLoadedSnippets());
}

TEST_F(AdblockSnippetStorageImplTest, StoreReplaceFile) {
  SnippetStorageImpl storage(temp_dir_.GetPath());
  std::string data = "StoreReplaceFile";
  std::string signature;
  storage.StoreSnippets(
      data, base::BindOnce(
                [](std::string* out, const std::string& sig) { *out = sig; },
                &signature));
  task_environment_.RunUntilIdle();
  std::string contents;
  EXPECT_TRUE(base::ReadFileToString(GetFileName(), &contents));
  EXPECT_EQ(data, contents);
  storage.LoadSnippets(signature);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(data, storage.GetLoadedSnippets());
  storage.LoadSnippets("");
  task_environment_.RunUntilIdle();
  EXPECT_EQ(data, storage.GetLoadedSnippets());
}

}  // namespace adblock
