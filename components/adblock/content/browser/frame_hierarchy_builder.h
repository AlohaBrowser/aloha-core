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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_HIERARCHY_BUILDER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_HIERARCHY_BUILDER_H_

#include <vector>

#include "base/sequence_checker.h"
#include "content/public/browser/render_frame_host.h"

class GURL;

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace adblock {
/**
 * @brief Builds the frame hierarchy based on the RenderFrameHost.
 * A frame hierarchy is an ordered list of URLs of frames containing
 * an element, beginning with the direct parent frame and ending with
 * the top-level URL of the page.
 * Frame hierarchies are traversed to find any allow rules that apply to
 * parent frames of blocked resource to override the blocking rule.
 * Used in browser process UI main thread.
 *
 */
class FrameHierarchyBuilder {
 public:
  FrameHierarchyBuilder();
  virtual ~FrameHierarchyBuilder();

  virtual content::RenderFrameHost* FindRenderFrameHost(
      content::GlobalRenderFrameHostId render_frame_host_id) const;

  virtual std::vector<GURL> BuildFrameHierarchy(
      content::RenderFrameHost* host) const;
  virtual GURL FindUrlForFrame(content::RenderFrameHost* host,
                               content::WebContents* web_contents) const;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_FRAME_HIERARCHY_BUILDER_H_
