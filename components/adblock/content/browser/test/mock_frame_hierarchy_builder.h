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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_FRAME_HIERARCHY_BUILDER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_FRAME_HIERARCHY_BUILDER_H_

#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockFrameHierarchyBuilder
    : public testing::NiceMock<FrameHierarchyBuilder> {
 public:
  MockFrameHierarchyBuilder();
  ~MockFrameHierarchyBuilder() override;

  MOCK_METHOD(content::RenderFrameHost*,
              FindRenderFrameHost,
              (content::GlobalRenderFrameHostId),
              (const, override));
  MOCK_METHOD(std::vector<GURL>,
              BuildFrameHierarchy,
              (content::RenderFrameHost*),
              (const, override));
  MOCK_METHOD(GURL,
              FindUrlForFrame,
              (content::RenderFrameHost*, content::WebContents*),
              (const, override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_FRAME_HIERARCHY_BUILDER_H_
