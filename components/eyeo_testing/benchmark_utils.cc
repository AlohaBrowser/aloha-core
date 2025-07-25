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

#include "components/eyeo_testing/benchmark_utils.h"

#include <iostream>
#include <string>

#include "absl/types/variant.h"
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/metrics/statistics_recorder.h"
#include "base/path_service.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/converter/flatbuffer_converter.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"
#include "google/compression_utils.h"

namespace adblock::test {

base::FilePath DefaultDataPath() {
  base::FilePath default_path;
  CHECK(base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &default_path));
  default_path = default_path.AppendASCII("components")
                     .AppendASCII("eyeo_testing")
                     .AppendASCII("benchmark_data");
  return default_path;
}

scoped_refptr<InstalledSubscription> LoadFilters(std::string_view file) {
  static scoped_refptr<FlatbufferConverter> converter =
      base::MakeRefCounted<FlatbufferConverter>();
  std::string content;
  CHECK(base::ReadFileToString(DefaultDataPath().AppendASCII(file), &content));
  CHECK(compression::GzipUncompress(content, &content));
  std::stringstream file_stream(content);
  CHECK(file_stream);
  const auto url = GURL("https://fakeurl.com").Resolve(file);
  const auto conversion_time = base::ElapsedTimer();
  auto flatbuffer_data = converter->Convert(file_stream, url, true);
  std::cout << "Conversion time for " << file << ": "
            << conversion_time.Elapsed() << std::endl;
  CHECK(absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
      flatbuffer_data));
  return base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(absl::get<std::unique_ptr<FlatbufferData>>(flatbuffer_data)),
      Subscription::InstallationState::Installed, base::Time::Now());
}

base::Value LoadRequests() {
  std::string content;
  CHECK(base::ReadFileToString(
      DefaultDataPath().AppendASCII("requests.json.gz"), &content));
  CHECK(compression::GzipUncompress(content, &content));
  auto parsed_value = base::JSONReader::Read(content);
  CHECK(parsed_value);
  return std::move(*parsed_value);
}

SubscriptionService::Snapshot MakeSnapshot(
    const std::vector<scoped_refptr<InstalledSubscription>>& subscriptions) {
  SubscriptionService::Snapshot snapshot;
  snapshot.push_back(
      std::make_unique<SubscriptionCollectionImpl>(subscriptions, "test"));
  return snapshot;
}

void PrintHistograms() {
  std::string histograms_output;
  base::StatisticsRecorder::WriteGraph("adblock", &histograms_output);
  std::cout << histograms_output << std::endl;
}

}  // namespace adblock::test
