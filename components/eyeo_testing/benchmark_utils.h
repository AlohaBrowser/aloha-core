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

#ifndef COMPONENTS_EYEO_TESTING_BENCHMARK_UTILS_H_
#define COMPONENTS_EYEO_TESTING_BENCHMARK_UTILS_H_

#include "base/values.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/subscription_service.h"

namespace adblock::test {

// Loads a filter list from  components/eyeo_testing/benchmark_data/|file|.
// The filter lists in that directory were downloaded from
// https://github.com/ghostery/adblocker/tree/master/packages/adblocker-benchmarks/lists
// from commit c8fc72e and gzipped.
scoped_refptr<InstalledSubscription> LoadFilters(std::string_view file);

// Loads a JSON file with pre-recorded requests. The form is like this:
// [{"frameUrl":"https://www.craigslist.org/",
//   "url":"https://www.craigslist.org/",
//   "cpt":"document"},
//  {"frameUrl":"https://steamcommunity.com/app/1120810",
//   "url":"https://community.akamai.steamstatic.com/...d9b057a1240ea9a220b",
//   "cpt":"stylesheet"},
//  ...
// ]
// This is loaded from components/eyeo_testing/benchmark_data/requests.json.gz.
// This is a subset of https://cdn.cliqz.com/adblocking/requests_top500.json.gz
// which is a big (9GB) file sometimes used for adblocker benchmarking and has
// the same format. It was downloaded from:
// https://raw.githubusercontent.com/mjethani/scaling-palm-tree/06ef031aa3d2742dc7e6234f579e28a9b6d499b0/requests.json
// and gzipped.
base::Value LoadRequests();

SubscriptionService::Snapshot MakeSnapshot(
    const std::vector<scoped_refptr<InstalledSubscription>>& subscriptions);

// Prints all histograms with "adblock." prefix to stdout.
void PrintHistograms();

}  // namespace adblock::test

#endif  // COMPONENTS_EYEO_TESTING_BENCHMARK_UTILS_H_
