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

#include "components/adblock/core/snippet/snippet_service_impl.h"

#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "components/adblock/core/snippet/test/mock_snippet_downloader.h"
#include "components/adblock/core/snippet/test/mock_snippet_storage.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/mock_resource_bundle_delegate.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace adblock {

class AdblockSnippetServiceImplTest : public testing::Test {
 public:
  void SetUp() override {
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", &mock_delegate_,
        ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
    EXPECT_CALL(mock_delegate_,
                LoadDataResourceString(IDR_ADBLOCK_SNIPPETS_XPATH3_DEP_JS))
        .WillRepeatedly(testing::Return("xpath3_dep"));
  }

  void TearDown() override { ui::ResourceBundle::CleanupSharedInstance(); }

  ui::ResourceBundle::SharedInstanceSwapperForTesting instance_swapper_;
  testing::NiceMock<ui::MockResourceBundleDelegate> mock_delegate_;
  std::string library = "// ! Version: 2.2.0\nsnippets_lib";
};

TEST_F(AdblockSnippetServiceImplTest, Default) {
  base::ListValue snippets_config;
  snippets_config.Append("test");
  auto storage = std::make_unique<MockSnippetStorage>();
  EXPECT_CALL(*storage, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(library));
  SnippetUpdateServiceImpl service(std::move(storage),
                                   std::make_unique<MockSnippetDownloader>());
  EXPECT_EQ(service.GenerateSnippetScript({}, std::move(snippets_config)),
            "(// ! Version: 2.2.0\nsnippets_lib)({}, ...[\"test\"]);");
}

TEST_F(AdblockSnippetServiceImplTest, InjectsXpath3DepWhenXpath3FilterFound) {
  base::ListValue snippets_config;
  snippets_config.Append("hide-if-matches-xpath3");
  auto storage = std::make_unique<MockSnippetStorage>();
  EXPECT_CALL(*storage, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(library));
  SnippetUpdateServiceImpl service(std::move(storage),
                                   std::make_unique<MockSnippetDownloader>());
  EXPECT_EQ(service.GenerateSnippetScript({}, std::move(snippets_config)),
            "(xpath3_dep)();(// ! Version: 2.2.0\nsnippets_lib)({}, "
            "...[\"hide-if-matches-xpath3\"]);");
}

TEST_F(AdblockSnippetServiceImplTest, Version) {
  auto storage = std::make_unique<MockSnippetStorage>();
  EXPECT_CALL(*storage, GetLoadedSnippets())
      .WillOnce(testing::ReturnRef(library));
  SnippetUpdateServiceImpl service(std::move(storage),
                                   std::make_unique<MockSnippetDownloader>());
  EXPECT_EQ(service.GetSnippetVersion(), "2.2.0");
}

}  // namespace adblock
