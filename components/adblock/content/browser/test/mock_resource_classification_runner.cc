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

#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "gtest/gtest.h"

namespace adblock {

MockResourceClassificationRunner::MockResourceClassificationRunner() = default;
MockResourceClassificationRunner::~MockResourceClassificationRunner() = default;

void MockResourceClassificationRunner::AddObserver(
    ResourceClassificationRunner::Observer* observer) {
  observers_.AddObserver(observer);
}

void MockResourceClassificationRunner::RemoveObserver(
    ResourceClassificationRunner::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MockResourceClassificationRunner::NotifyResourceMatched(
    const GURL& url,
    FilterMatchResult result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription) {
  for (auto& observer : observers_) {
    observer.OnRequestMatched(url, result, parent_frame_urls, content_type,
                              render_frame_host, subscription, "");
  }
}

}  // namespace adblock
