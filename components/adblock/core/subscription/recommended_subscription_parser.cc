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

#include <string>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"

namespace adblock {

// static
std::vector<GURL> RecommendedSubscriptionParser::FromFile(
    base::FilePath downloaded_file) {
  std::string recommendation_json_string;
  auto read_result =
      base::ReadFileToString(downloaded_file, &recommendation_json_string);
  base::DeleteFile(downloaded_file);
  if (!read_result) {
    LOG(ERROR) << "[eyeo] Could not read recommended subscription list";
    return {};
  }

  VLOG(1) << "[eyeo] Raw subscription recommendations: "
          << recommendation_json_string;

  auto recommendation_json = base::JSONReader::ReadAndReturnValueWithError(
      recommendation_json_string, base::JSON_PARSE_RFC);

  if (!recommendation_json.has_value()) {
    LOG(ERROR) << "[eyeo] Could not parse recommended subscription list: "
               << recommendation_json.error().ToString();
    return {};
  }

  if (!recommendation_json->is_list()) {
    LOG(ERROR) << "[eyeo] Invalid recommended subscription data";
    return {};
  }

  std::vector<GURL> recommended_subscription_urls;
  auto& recommended_subscriptions = recommendation_json->GetList();
  for (auto& recommended_subscription : recommended_subscriptions) {
    if (!recommended_subscription.is_dict()) {
      LOG(ERROR) << "[eyeo] Invalid recommended subscription data";
      continue;
    }

    std::string* recommended_subscription_url =
        recommended_subscription.GetDict().FindString("url");
    if (!recommended_subscription_url) {
      LOG(ERROR) << "[eyeo] Invalid recommended subscription data";
      continue;
    }

    VLOG(1) << "[eyeo] Recommended subscription url: "
            << *recommended_subscription_url;
    recommended_subscription_urls.emplace_back(*recommended_subscription_url);
  }

  return recommended_subscription_urls;
}

}  // namespace adblock
