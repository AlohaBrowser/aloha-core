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

#include "components/adblock/content/browser/frame_hierarchy_builder.h"

#include <cstdint>

#include "components/adblock/content/browser/request_initiator.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

class AdblockFrameHierarchyTest : public content::RenderViewHostTestHarness {
 public:
  AdblockFrameHierarchyTest() = default;
  ~AdblockFrameHierarchyTest() override = default;

  AdblockFrameHierarchyTest(const AdblockFrameHierarchyTest&) = delete;
  AdblockFrameHierarchyTest& operator=(const AdblockFrameHierarchyTest&) =
      delete;

  content::RenderFrameHost* AddChildFrame(content::RenderFrameHost* parent,
                                          const GURL& url) {
    content::RenderFrameHost* child =
        content::RenderFrameHostTester::For(parent)->AppendChild("");
    content::RenderFrameHostTester::For(child)->InitializeRenderFrameIfNeeded();
    auto simulator =
        content::NavigationSimulator::CreateRendererInitiated(url, child);
    simulator->Commit();
    return simulator->GetFinalRenderFrameHost();
  }

  std::vector<GURL> BuildFrameHierarchy(const GURL& root,
                                        std::initializer_list<GURL> urls) {
    NavigateAndCommit(root);
    auto* frame = main_rfh();

    for (const GURL& it : urls) {
      frame = AddChildFrame(frame, it);
    }

    adblock::FrameHierarchyBuilder builder;
    return builder.BuildFrameHierarchy(adblock::RequestInitiator(frame));
  }
};

}  // namespace

namespace adblock {

const char kRoot[] = "https://foo.com/root";
const char kRootFile[] = "file:///some/path/to.file";
const char kFrame1[] = "https://foo.com/frame1";
const char kFrame2[] = "https://foo.com/frame2";
const char kBlank[] = "about:blank";

TEST_F(AdblockFrameHierarchyTest, Build) {
  GURL root(kRoot);
  GURL url1(kFrame1);
  GURL url2(kFrame2);
  auto result = BuildFrameHierarchy(root, {url1, url2});

  ASSERT_EQ(3u, result.size());
  EXPECT_EQ(url2, result.at(0));
  EXPECT_EQ(url1, result.at(1));
  EXPECT_EQ(root, result.at(2));
}

TEST_F(AdblockFrameHierarchyTest, BuildForFile) {
  GURL root(kRootFile);
  GURL url1(kFrame1);
  GURL url2(kFrame2);
  auto result = BuildFrameHierarchy(root, {url1, url2});

  ASSERT_EQ(3u, result.size());
  EXPECT_EQ(url2, result.at(0));
  EXPECT_EQ(url1, result.at(1));
  EXPECT_EQ(kRootFile, result.at(2));
}

TEST_F(AdblockFrameHierarchyTest, BuildBlank) {
  GURL root(kRoot);
  GURL url1(kBlank);
  GURL url2(kFrame2);
  auto result = BuildFrameHierarchy(root, {url1, url2});

  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(url2, result.at(0));
  EXPECT_EQ(root, result.at(1));
}

TEST_F(AdblockFrameHierarchyTest, BuildEmpty) {
  GURL root(kRoot);
  GURL url1(kFrame1);
  GURL url2;
  auto result = BuildFrameHierarchy(root, {url1, url2});

  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(url1, result.at(0));
  EXPECT_EQ(root, result.at(1));
}

TEST_F(AdblockFrameHierarchyTest, BuildForDetachedRequest) {
  const GURL url(kFrame1);

  FrameHierarchyBuilder builder;
  const auto result = builder.BuildFrameHierarchy(RequestInitiator(url));
  EXPECT_EQ(result, std::vector<GURL>({url}));
}

}  // namespace adblock
