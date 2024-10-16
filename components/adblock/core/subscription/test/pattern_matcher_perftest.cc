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
#include "components/adblock/core/subscription/pattern_matcher.h"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <vector>

#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/subscription/test/load_gzipped_test_file.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_result_reporter.h"
#include "url/gurl.h"

namespace adblock {
namespace {
constexpr char kMetricRuntime[] = ".runtime";

void MatchPatterns(const std::vector<std::string_view>& patterns,
                   const GURL& url) {
  for (const auto p : patterns) {
    DoesPatternMatchUrl(p, url);
  }
}

std::string GetTestName() {
  auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
  return std::string(test_info->test_suite_name()) + "." + test_info->name();
}
}  // namespace

TEST(AdblockPatternMatcherPerfTest, FilterMatchingSpeed) {
  const auto url_file_content = LoadGzippedTestFile("5000_urls.txt.gz");
  std::vector<GURL> urls;
  base::ranges::transform(
      base::SplitStringPiece(url_file_content, "\n", base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY),
      std::back_inserter(urls),
      [](const auto string_piece) { return GURL(string_piece); });
  const auto pattern_file_content = LoadGzippedTestFile("5000_patterns.txt.gz");
  const auto patterns =
      base::SplitStringPiece(pattern_file_content, "\n", base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);
  perf_test::PerfResultReporter reporter(GetTestName(),
                                         "5000 patterns, 5000 urls");
  reporter.RegisterImportantMetric(kMetricRuntime, "ms");
  base::ElapsedTimer timer;
  for (const auto& url : urls) {
    MatchPatterns(patterns, url);
  }
  reporter.AddResult(kMetricRuntime,
                     static_cast<size_t>(timer.Elapsed().InMilliseconds()));
}

}  // namespace adblock
