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

#include <iostream>
#include <memory>

#include "absl/types/variant.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/timer/elapsed_timer.h"
#include "base/values.h"
#include "components/adblock/core/classifier/resource_classifier_impl.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/eyeo_testing/benchmark_utils.h"

using namespace adblock;

absl::optional<ContentType> StringToContentType(
    std::string_view content_type_string) {
  static base::flat_map<std::string_view, ContentType> kMapping = {
      {"font", ContentType::Font},
      {"image", ContentType::Image},
      {"media", ContentType::Media},
      {"other", ContentType::Other},
      {"script", ContentType::Script},
      {"stylesheet", ContentType::Stylesheet},
      {"xhr", ContentType::Xmlhttprequest},
      {"fetch", ContentType::Xmlhttprequest},
      {"eventsource", ContentType::Other},
      {"ping", ContentType::Ping},
      {"manifest", ContentType::Other},
      {"texttrack", ContentType::Other}};
  auto it = kMapping.find(content_type_string);
  if (it != kMapping.end()) {
    return it->second;
  }
  return absl::nullopt;
}

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const auto installed_subscriptions =
      std::vector<scoped_refptr<InstalledSubscription>>{
          test::LoadFilters("easylist-benchmark.txt.gz"),
          test::LoadFilters("easyprivacy-benchmark.txt.gz"),
      };

  const auto classifier = base::MakeRefCounted<ResourceClassifierImpl>();
  const auto requests = test::LoadRequests();
  CHECK(requests.is_list());
  std::cout << "Starting benchmark, " << requests.GetList().size()
            << " requests" << std::endl;
  // For each request, classify it and measure elapsed time.
  for (const auto& request : requests.GetList()) {
    CHECK(request.is_dict());
    const auto& dict = request.GetDict();
    const auto url = GURL(*dict.FindString("url"));
    const auto frame_url = GURL(*dict.FindString("frameUrl"));
    const auto content_type_string = *dict.FindString("cpt");
    const auto content_type = StringToContentType(content_type_string);
    if (!content_type) {
      continue;
    }
    const auto request_time = base::ElapsedTimer();
    classifier->ClassifyRequest(test::MakeSnapshot(installed_subscriptions),
                                url, {frame_url}, *content_type, {});
    UMA_HISTOGRAM_CUSTOM_MICROSECONDS_TIMES(
        "adblock.classification_time", request_time.Elapsed(),
        base::Microseconds(1), base::Microseconds(5000), 200);
  }

  test::PrintHistograms();
  return 0;
}
