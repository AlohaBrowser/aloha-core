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
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/classifier/resource_classifier_impl.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"
#include "components/adblock/core/subscription/subscription_service.h"
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

class AdblockResourceClassifierPerfTestBase : public testing::Test {
 public:
  void SetUp() override {
    classifier_ = base::MakeRefCounted<ResourceClassifierImpl>();
    converter_ = base::MakeRefCounted<FlatbufferConverter>();
  }

  virtual std::string GetTimerResolutionUnits() const = 0;
  virtual void AddResult(perf_test::PerfResultReporter& reporter,
                         const base::ElapsedTimer& timer) const = 0;

  static std::string ReadFromTestData(const std::string& file_name) {
    base::FilePath source_file;
    EXPECT_TRUE(
        base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &source_file));
    source_file = source_file.AppendASCII("components")
                      .AppendASCII("test")
                      .AppendASCII("data")
                      .AppendASCII("adblock")
                      .AppendASCII(file_name);
    std::string content;
    CHECK(base::ReadFileToString(source_file, &content));
    CHECK(compression::GzipUncompress(content, &content));
    return content;
  }

  static int BenchmarkRepetitions() {
    // Android devices are much slower than Desktop computers, reduce the
    // number of reps so they don't time out. On Desktop, a higher number of
    // reps allows more reliable measurement via perf tools.
#if BUILDFLAG(IS_ANDROID)
    return 50;
#else
    return 500;
#endif
  }

  void MeasureUrlMatchingTime(
      GURL url,
      ContentType content_type,
      std::vector<scoped_refptr<InstalledSubscription>> state,
      int cycles = BenchmarkRepetitions()) {
    ResourceClassifier::ClassificationResult classification_result;
    perf_test::PerfResultReporter reporter(GetTestName(), "url_matching");
    reporter.RegisterImportantMetric(kMetricRuntime, GetTimerResolutionUnits());
    base::ElapsedTimer timer;
    // Call matching many times to make sure perf woke up for measurement.
    for (int i = 0; i < cycles; ++i) {
      SubscriptionService::Snapshot snapshot;
      snapshot.emplace_back(
          std::make_unique<SubscriptionCollectionImpl>(state, ""));
      classification_result = classifier_->ClassifyRequest(
          std::move(snapshot), url, DefaultFrameHierarchy(), content_type,
          DefaultSitekey());
    }
    AddResult(reporter, timer);
    VLOG(1) << "Classification result: "
            << ClassificationResultToString(classification_result);
  }

  void MeasureCSPMatchingTime(
      GURL url,
      ContentType content_type,
      std::vector<scoped_refptr<InstalledSubscription>> state,
      int cycles = BenchmarkRepetitions()) {
    std::set<std::string_view> csp_injections;
    auto sub_collection =
        std::make_unique<SubscriptionCollectionImpl>(state, "");
    perf_test::PerfResultReporter reporter(GetTestName(), "csp_matching");
    reporter.RegisterImportantMetric(kMetricRuntime, GetTimerResolutionUnits());
    base::ElapsedTimer timer;
    // Call matching many times to make sure perf woke up for measurement.
    for (int i = 0; i < cycles; ++i) {
      csp_injections =
          sub_collection->GetCspInjections(url, DefaultFrameHierarchy());
    }
    AddResult(reporter, timer);
    for (const auto& csp_i : csp_injections) {
      VLOG(1) << "CSP injection found: " << csp_i;
    }
  }

  void MeasureElemhideGenerationTime(
      GURL url,
      std::vector<scoped_refptr<InstalledSubscription>> state) {
    auto sub_collection =
        std::make_unique<SubscriptionCollectionImpl>(state, "");
    perf_test::PerfResultReporter reporter(GetTestName(),
                                           "elemhide_generation");
    reporter.RegisterImportantMetric(kMetricRuntime, GetTimerResolutionUnits());
    base::ElapsedTimer timer;
    // Call generation many times to make sure perf woke up for measurement.
    for (int i = 0; i < BenchmarkRepetitions(); ++i) {
      sub_collection->GetElementHideData(url, DefaultFrameHierarchy(),
                                         DefaultSitekey());
    }
    AddResult(reporter, timer);
  }

  const GURL& UnknownAddress() const {
    static const GURL kUnknownAddress{
        "https://eyeo.com/themes/custom/eyeo_theme/logo.svg"};
    return kUnknownAddress;
  }

  const GURL& BlockedAddress() const {
    static const GURL kBlockedAddress{"https://0265331.com/whatever/image.png"};
    return kBlockedAddress;
  }

  const std::vector<GURL>& DefaultFrameHierarchy() const {
    static const std::vector<GURL> kFrameHierarchy{
        GURL("https://frame.com/frame1.html"),
        GURL("https://frame.com/frame2.html"),
        GURL("https://frame.com/"),
    };
    return kFrameHierarchy;
  }

  const SiteKey& DefaultSitekey() const {
    static const SiteKey kSiteKey{"abc"};
    return kSiteKey;
  }

  std::string_view ClassificationResultToString(
      const ResourceClassifier::ClassificationResult& result) {
    switch (result.decision) {
      case ResourceClassifier::ClassificationResult::Decision::Allowed:
        return "Allowed";
      case ResourceClassifier::ClassificationResult::Decision::Blocked:
        return "Blocked";
      case ResourceClassifier::ClassificationResult::Decision::Ignored:
        return "Ignored";
    }
    return "";
  }

  scoped_refptr<ResourceClassifier> classifier_;
  scoped_refptr<FlatbufferConverter> converter_;
};

// Uses real filter lists
class AdblockResourceClassifierPerfTestFull
    : public AdblockResourceClassifierPerfTestBase {
 public:
  std::vector<scoped_refptr<InstalledSubscription>> CreateState(
      std::initializer_list<std::string> filenames) {
    std::vector<scoped_refptr<InstalledSubscription>> state;
    for (const auto& cur : filenames) {
      const std::string content = ReadFromTestData(cur);
      std::stringstream input(std::move(content));
      auto converter_result =
          converter_->Convert(input, CustomFiltersUrl(), false);
      DCHECK(absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
          converter_result));
      state.push_back(base::MakeRefCounted<InstalledSubscriptionImpl>(
          std::move(
              absl::get<std::unique_ptr<FlatbufferData>>(converter_result)),
          Subscription::InstallationState::Installed, base::Time()));
    }
    return state;
  }

  std::string GetTimerResolutionUnits() const override { return "ms"; }
  void AddResult(perf_test::PerfResultReporter& reporter,
                 const base::ElapsedTimer& timer) const override {
    reporter.AddResult(kMetricRuntime,
                       static_cast<size_t>(timer.Elapsed().InMilliseconds()));
  }
};

TEST_F(AdblockResourceClassifierPerfTestFull, UrlNoMatch) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
  MeasureUrlMatchingTime(UnknownAddress(), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestFull, UrlBlocked) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
  MeasureUrlMatchingTime(BlockedAddress(), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestFull, ElemhideNoMatch) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
  MeasureElemhideGenerationTime(UnknownAddress(), std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestFull, ElemhideMatch) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
  MeasureElemhideGenerationTime(
      GURL{"https://www.heise.de/news/"
           "Privacy-Shield-2-0-Viele-offene-Fragen-zum-Datenverkehr-mit-den-"
           "USA-6658370.html"},
      std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestFull, LongUrlMatch) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
  // "longurl.txt.gz" contains a 300K-long URL that encodes a ton of debug data.
  // This URL was recorded from a real site. It can be orders of magnitude
  // slower to match than typical URLs so we use a custom repetition count.
#if BUILDFLAG(IS_ANDROID)
  const int kRepCount = 5;
#else
  const int kRepCount = 50;
#endif
  const GURL long_url(ReadFromTestData("longurl.txt.gz"));
  MeasureUrlMatchingTime(long_url, ContentType::Subdocument, std::move(state),
                         kRepCount);
}

TEST_F(AdblockResourceClassifierPerfTestFull, LongUrlFindCsp) {
  auto state = CreateState({"easylist.txt.gz", "exceptionrules.txt.gz"});
#if BUILDFLAG(IS_ANDROID)
  const int kRepCount = 5;
#else
  const int kRepCount = 50;
#endif
  const GURL long_url(ReadFromTestData("longurl.txt.gz"));
  MeasureCSPMatchingTime(long_url, ContentType::Subdocument, std::move(state),
                         kRepCount);
}

// Uses one or just a couple of filters
class AdblockResourceClassifierPerfTestSimple
    : public AdblockResourceClassifierPerfTestBase {
 public:
  std::vector<scoped_refptr<InstalledSubscription>> CreateStateFromFilters(
      std::vector<std::string> filters) {
    auto converter_result =
        converter_->Convert(std::move(filters), CustomFiltersUrl(), false);
    DCHECK(converter_result);
    std::vector<scoped_refptr<InstalledSubscription>> state;
    state.push_back(base::MakeRefCounted<InstalledSubscriptionImpl>(
        std::move(converter_result), Subscription::InstallationState::Installed,
        base::Time()));
    return state;
  }

  std::string GetTimerResolutionUnits() const override { return "us"; }
  void AddResult(perf_test::PerfResultReporter& reporter,
                 const base::ElapsedTimer& timer) const override {
    reporter.AddResult(kMetricRuntime, timer.Elapsed());
  }
};

TEST_F(AdblockResourceClassifierPerfTestSimple, NoWildcardDomainElemhideMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"example.com###selector"});
  MeasureElemhideGenerationTime(GURL("https://example.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple,
       NoWildcardMultipleDomainsElemhideMatch) {
  auto state = CreateStateFromFilters(std::vector<std::string>{
      "example.de,example.pl,example.net,example.co.uk,example.com,example.fr, "
      "example.hu###selector"});
  MeasureElemhideGenerationTime(GURL("https://example.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, WildcardDomainElemhideMatch) {
  auto state =
      CreateStateFromFilters(std::vector<std::string>{"example.*###selector"});
  MeasureElemhideGenerationTime(GURL("https://example.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple,
       NoWildcardDomainElemhideNoMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"example.com###selector"});
  MeasureElemhideGenerationTime(GURL("https://wrong.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple,
       NoWildcardMultipleDomainsElemhideNoMatch) {
  auto state = CreateStateFromFilters(std::vector<std::string>{
      "example.de,example.pl,example.net,example.co.uk,example.com,example.fr, "
      "example.hu###selector"});
  MeasureElemhideGenerationTime(GURL("https://wrong.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, WildcardDomainElemhideNoMatch) {
  auto state =
      CreateStateFromFilters(std::vector<std::string>{"example.*###selector"});
  MeasureElemhideGenerationTime(GURL("https://wrong.com/frame.html"),
                                std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, NoWildcardDomainUrlMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"ad.png$domain=frame.com"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple,
       NoWildcardMultipleDomainsUrlMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"ad.png$domain=frame.de|frame.pl|frame.net|"
                               "frame.co.uk|frame.com|frame.fr|frame.hu"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, WildcardDomainUrlMatch) {
  auto state =
      CreateStateFromFilters(std::vector<std::string>{"ad.png$domain=frame.*"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, NoWildcardDomainUrlNoMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"ad.png$domain=wrong.com"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple,
       NoWildcardMultipleDomainsUrlNoMatch) {
  auto state = CreateStateFromFilters(
      std::vector<std::string>{"ad.png$domain=wrong.de|wrong.pl|wrong.net|"
                               "wrong.co.uk|wrong.com|wrong.fr|wrong.hu"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

TEST_F(AdblockResourceClassifierPerfTestSimple, WildcardDomainUrlNoMatch) {
  auto state =
      CreateStateFromFilters(std::vector<std::string>{"ad.png$domain=wrong.*"});

  MeasureUrlMatchingTime(GURL("https://example.com/ad.png"), ContentType::Image,
                         std::move(state));
}

}  // namespace adblock
