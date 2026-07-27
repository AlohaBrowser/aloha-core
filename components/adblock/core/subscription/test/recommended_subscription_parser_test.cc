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

#include "components/adblock/core/subscription/recommended_subscription_parser.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockRecommendedSubscriptionParserTest : public testing::Test {
 public:
  void Test(const std::string& downloaded_file_content,
            const std::vector<GURL>& expected_result) {
    // Mock downloaded file
    base::FilePath recommendations_tmp_file;
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(),
                                               &recommendations_tmp_file));
    ASSERT_TRUE(
        base::WriteFile(recommendations_tmp_file, downloaded_file_content));

    EXPECT_THAT(expected_result,
                testing::ContainerEq(RecommendedSubscriptionParser::FromFile(
                    recommendations_tmp_file)));

    task_environment_.RunUntilIdle();
    EXPECT_FALSE(base::PathExists(recommendations_tmp_file));
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(AdblockRecommendedSubscriptionParserTest, EmptyFile) {
  std::string downloaded_file_content = "";
  std::vector<GURL> expected_result = {};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest, ValidListReturned) {
  std::string downloaded_file_content =
      "[{\"url\": \"https://recommended-fl/list1.txt\"}, {\"url\": "
      "\"https://recommended-fl/list2.txt\"}]";
  std::vector<GURL> expected_result = {
      GURL("https://recommended-fl/list1.txt"),
      GURL("https://recommended-fl/list2.txt")};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest,
       InvalidRecommendationData_JsonSyntaxError) {
  std::string downloaded_file_content = "not : valid : json";
  std::vector<GURL> expected_result = {};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest,
       InvalidRecommendationData_NotAList) {
  std::string downloaded_file_content =
      "{\"url\": \"https://recommended-fl/list1.txt\"}";
  std::vector<GURL> expected_result = {};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest,
       InvalidRecommendationData_NotADict) {
  std::string downloaded_file_content =
      "[[{\"url\": \"https://recommended-fl/list1.txt\"}]]";
  std::vector<GURL> expected_result = {};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest,
       InvalidRecommendationData_NoUrlKey) {
  std::string downloaded_file_content =
      "[{\"not_url\": \"https://recommended-fl/list1.txt\"}]";
  std::vector<GURL> expected_result = {};

  Test(downloaded_file_content, expected_result);
}

TEST_F(AdblockRecommendedSubscriptionParserTest,
       ValidAndInvalidRecommendationsMixed) {
  std::string downloaded_file_content =
      "[{\"not_url\": \"https://recommended-fl/list1.txt\"}, {\"url\": "
      "\"https://recommended-fl/list2.txt\"}]";
  std::vector<GURL> expected_result = {
      GURL("https://recommended-fl/list2.txt")};

  Test(downloaded_file_content, expected_result);
}

}  // namespace adblock
