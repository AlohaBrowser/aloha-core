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

#include <sstream>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_result_reporter.h"
#include "third_party/zlib/google/compression_utils.h"

namespace adblock {

namespace {
constexpr char kMetricRuntime[] = ".runtime";

std::string GetTestName() {
  auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
  return std::string(test_info->test_suite_name()) + "." + test_info->name();
}
}  // namespace

class AdblockConverterPerfTest : public testing::Test {
 public:
  void SetUp() override {
    converter_ = base::MakeRefCounted<FlatbufferConverter>();
  }
  void MeasureConversionTime(std::string filename) {
    base::FilePath source_file;
    ASSERT_TRUE(
        base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &source_file));
    source_file = source_file.AppendASCII("components")
                      .AppendASCII("test")
                      .AppendASCII("data")
                      .AppendASCII("adblock")
                      .AppendASCII(filename);
    std::string content;
    ASSERT_TRUE(base::ReadFileToString(source_file, &content));
    ASSERT_TRUE(compression::GzipUncompress(content, &content));
    std::stringstream input(std::move(content));
    perf_test::PerfResultReporter reporter(GetTestName(), filename.c_str());
    reporter.RegisterImportantMetric(kMetricRuntime, "ms");
    base::ElapsedTimer timer;
    auto buffer = converter_->Convert(input, CustomFiltersUrl(), true);
    ASSERT_TRUE(
        absl::holds_alternative<std::unique_ptr<FlatbufferData>>(buffer));
    reporter.AddResult(kMetricRuntime,
                       static_cast<size_t>(timer.Elapsed().InMilliseconds()));
  }

 private:
  scoped_refptr<FlatbufferConverter> converter_ =
      base::MakeRefCounted<FlatbufferConverter>();
};

TEST_F(AdblockConverterPerfTest, ConvertEasylistTime) {
  MeasureConversionTime("easylist.txt.gz");
}

TEST_F(AdblockConverterPerfTest, ConvertExceptionrulesTime) {
  MeasureConversionTime("exceptionrules.txt.gz");
}

}  // namespace adblock
