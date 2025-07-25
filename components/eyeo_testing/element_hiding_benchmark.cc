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

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/timer/elapsed_timer.h"
#include "base/values.h"
#include "components/adblock/core/classifier/resource_classifier_impl.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/eyeo_testing/benchmark_utils.h"

using namespace adblock;

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const auto subscription_collection =
      std::make_unique<SubscriptionCollectionImpl>(
          std::vector<scoped_refptr<InstalledSubscription>>{
              test::LoadFilters("easylist-benchmark.txt.gz"),
              test::LoadFilters("easyprivacy-benchmark.txt.gz"),
          },
          "test");

  const auto requests = test::LoadRequests();
  // Collect all unique frame URLs from the requests.
  std::unordered_set<std::string> frame_urls;
  for (const auto& request : requests.GetList()) {
    CHECK(request.is_dict());
    const auto& dict = request.GetDict();
    frame_urls.insert(*dict.FindString("frameUrl"));
  }

  // For each frame URL, collect element hiding selectors from the
  // subscription_collection and measure the time.
  std::cout << "Collecting element hiding selectors for " << frame_urls.size()
            << " frames" << std::endl;
  for (const auto& frame_url : frame_urls) {
    const auto request_time = base::ElapsedTimer();
    // Empty frame hierarchy, empty sitekey, this is typical for top-level
    // frames.
    subscription_collection->GetElementHideData(GURL(frame_url), {}, {});
    UMA_HISTOGRAM_CUSTOM_MICROSECONDS_TIMES(
        "adblock.element_hiding_selectors_time", request_time.Elapsed(),
        base::Microseconds(1), base::Microseconds(5000), 200);
  }
  test::PrintHistograms();
  return 0;
}
