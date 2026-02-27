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

#ifndef COMPONENTS_ADBLOCK_CORE_CLASSIFIER_TEST_MOCK_RESOURCE_CLASSIFIER_H_
#define COMPONENTS_ADBLOCK_CORE_CLASSIFIER_TEST_MOCK_RESOURCE_CLASSIFIER_H_

#include "components/adblock/core/classifier/resource_classifier.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace adblock {

class MockResourceClassifier : public ResourceClassifier {
 public:
  MockResourceClassifier();
  MOCK_METHOD(ClassificationResult,
              ClassifyRequest,
              (const SubscriptionService::Snapshot,
               const GURL&,
               const std::vector<GURL>&,
               ContentType,
               const SiteKey&),
              (override, const));
  MOCK_METHOD(ClassificationResult,
              ClassifyPopup,
              (const SubscriptionService::Snapshot&,
               const GURL&,
               const std::vector<GURL>&,
               const SiteKey&),
              (override, const));
  MOCK_METHOD(ClassificationResult,
              ClassifyResponse,
              (const SubscriptionService::Snapshot,
               const GURL&,
               const std::vector<GURL>&,
               ContentType,
               const scoped_refptr<net::HttpResponseHeaders>&),
              (override, const));
  MOCK_METHOD(absl::optional<GURL>,
              CheckRewrite,
              (const SubscriptionService::Snapshot,
               const GURL&,
               const std::vector<GURL>&),
              (override, const));

 protected:
  ~MockResourceClassifier() override;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CLASSIFIER_TEST_MOCK_RESOURCE_CLASSIFIER_H_
