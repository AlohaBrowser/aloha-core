// Copyright 2026 Aloha Mobile Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
//
// ALOHA https://app.clickup.com/t/2558578/z93epac4kc
// Finds subtitles a player draws in its own DOM (not in textTracks) during a
// fullscreen session and fills the DOM half of SubtitleProbeReport.

#pragma once

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "base/time/time.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/mutation_observer.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"

namespace aloha {

// Player family from class/id/tag markers on the <video> and its ancestors;
// "unknown" when nothing matches.
std::string DetectPlayerFamily(blink::Element& video);

class SubtitleTextObserver;

class SubtitleProbe {
 public:
  static SubtitleProbe* GetInstance();

  SubtitleProbe(const SubtitleProbe&) = delete;
  SubtitleProbe& operator=(const SubtitleProbe&) = delete;

  // |requested| is the element the page asked to go fullscreen.
  void OnEnterFullscreen(blink::Element& requested,
                         blink::HTMLVideoElement& video);

  // Sends the report and stops observing. With |fullscreen_element| set, only
  // a session of that element (the video or a container around it) is ended.
  void Stop(const char* reason, blink::Element* fullscreen_element = nullptr);

 private:
  friend class base::NoDestructor<SubtitleProbe>;
  SubtitleProbe();
  ~SubtitleProbe();

  blink::Element* PlayerRoot(blink::Element& requested,
                             blink::HTMLVideoElement& video);
  void ScanOnce(blink::Element& root, blink::HTMLVideoElement& video);
  void SendReport(const char* reason, SubtitleTextObserver* delegate);

  blink::Persistent<blink::MutationObserver> observer_;
  blink::Persistent<SubtitleTextObserver> delegate_;
  blink::Persistent<blink::HTMLVideoElement> video_;
  // Observed besides the root: MutationObserver does not cross shadow
  // boundaries, and some players keep the caption layer outside the root.
  std::vector<blink::Persistent<blink::ShadowRoot>> shadow_roots_;
  std::vector<blink::Persistent<blink::Element>> extra_targets_;
  int marker_hits_ = 0;  // caption-marker elements with text over the video
  bool player_known_ = false;
  bool video_box_empty_ = false;
  base::TimeTicks session_start_;
};

}  // namespace aloha
