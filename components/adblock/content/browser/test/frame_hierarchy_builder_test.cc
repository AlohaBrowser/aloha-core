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
    return builder.BuildFrameHierarchy(frame);
  }
};

}  // namespace

namespace adblock {

const char kRoot[] = "https://foo.com/root";
const char kFrame1[] = "https://foo.com/frame1";
const char kFrame2[] = "https://foo.com/frame2";
const char kBlank[] = "about:blank";

TEST_F(AdblockFrameHierarchyTest, FindMainFrame) {
  FrameHierarchyBuilder builder;
  auto* host = builder.FindRenderFrameHost(main_rfh()->GetGlobalId());
  ASSERT_TRUE(host);
  EXPECT_EQ(host, main_rfh());
}

TEST_F(AdblockFrameHierarchyTest, FindChildFrame) {
  FrameHierarchyBuilder builder;
  NavigateAndCommit(GURL(kRoot));
  auto* child_frame = AddChildFrame(main_rfh(), GURL(kFrame1));
  CHECK(child_frame);
  CHECK(child_frame->GetProcess());
  auto* host = builder.FindRenderFrameHost(child_frame->GetGlobalId());
  ASSERT_TRUE(host);
  EXPECT_EQ(host, child_frame);
}

TEST_F(AdblockFrameHierarchyTest, CannotFindUnknownIds) {
  FrameHierarchyBuilder builder;
  // There is an ordinary RenderFrame...
  NavigateAndCommit(GURL(kRoot));
  auto* child_frame = AddChildFrame(main_rfh(), GURL(kFrame1));
  CHECK(child_frame);
  CHECK(child_frame->GetProcess());
  // There is also a frame bound to a WebContents in the Browser Process...
  auto web_contents = CreateTestWebContents();
  CHECK(web_contents);
  CHECK(web_contents->GetPrimaryMainFrame());
  // But we ask for a RFH that's none of these:
  const int32_t routing_id = 42;
  const int32_t process_id = 30;
  CHECK_NE(routing_id, main_rfh()->GetRoutingID());
  CHECK_NE(routing_id, child_frame->GetRoutingID());
  CHECK_NE(routing_id,
           web_contents->GetPrimaryMainFrame()->GetFrameTreeNodeId());
  CHECK_NE(process_id, main_rfh()->GetProcess()->GetID());
  CHECK_NE(process_id, network::mojom::kBrowserProcessId);

  // It's impossible to find a RFH for those parameters.
  auto* host = builder.FindRenderFrameHost(
      content::GlobalRenderFrameHostId(process_id, routing_id));
  EXPECT_FALSE(host);
}

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

}  // namespace adblock
