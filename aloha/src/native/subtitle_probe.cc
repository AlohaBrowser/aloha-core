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

#include "aloha/src/native/subtitle_probe.h"

#include <cstdint>
#include <string_view>

#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "media/mojo/mojom/media_player.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_mutation_observer_init.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/mutation_record.h"
#include "third_party/blink/renderer/core/dom/node_traversal.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/static_node_list.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "ui/gfx/geometry/rect.h"

namespace aloha {

namespace {

// Budgets: the observer must never become a CPU sink on a busy page.
constexpr base::TimeDelta kObserveFor = base::Seconds(90);
constexpr int kMaxRecordsTotal = 3000;
constexpr int kMaxCandidatesPerDelivery = 10;
constexpr int kMaxElementsPerDelivery = 30;
constexpr int kMaxScannedElements = 6000;
constexpr int kMaxDocumentElements = 10000;
constexpr size_t kMaxShadowRoots = 256;
constexpr size_t kMaxExtraTargets = 8;
constexpr size_t kMaxText = 120;

std::string Lower(const blink::String& s) {
  return base::ToLowerASCII(s.Utf8());
}

std::string TextOf(const blink::Element& el) {
  std::string text = const_cast<blink::Element&>(el)
                         .textContent()
                         .SimplifyWhiteSpace()
                         .Utf8();
  if (text.size() > kMaxText)
    text.resize(kMaxText);
  return text;
}

// Time readouts ("0:12 / 3:45"), bare numbers, punctuation.
bool LooksLikeControlText(std::string_view text) {
  if (text.size() < 2)
    return true;
  if (base::IsAsciiDigit(text.front()) &&
      text.substr(0, 9).find(':') != std::string_view::npos) {
    return true;
  }
  for (char c : text) {
    if (base::IsAsciiAlpha(c) || static_cast<unsigned char>(c) >= 0x80)
      return false;
  }
  return true;
}

int64_t Area(const gfx::Rect& r) {
  return static_cast<int64_t>(r.width()) * r.height();
}

// PlayerYouTube rewrites the <video> style on entry and leaves it with an
// empty box; the player root stands in then.
gfx::Rect VideoArea(blink::HTMLVideoElement& video, blink::Element& root) {
  gfx::Rect box = video.BoundsInWidget();
  return box.IsEmpty() ? root.BoundsInWidget() : box;
}

bool HasCaptionMarker(std::string_view haystack) {
  static constexpr std::string_view kTokens[] = {
      "caption", "subtitle", "cue",       "texttrack", "text-track",
      "libass",  "jassub",   "octopus",   "subs-",     "-subs",
      "_subs",   "subs_",    "ytp-caption",
  };
  for (std::string_view token : kTokens) {
    if (haystack.find(token) != std::string_view::npos)
      return true;
  }
  return false;
}

// Tag included: web components name the caption layer by element
// (<smp-subtitles>, <media-captions>).
bool ElementHasCaptionMarker(const blink::Element& el) {
  if (HasCaptionMarker(Lower(el.localName())))
    return true;
  const blink::AtomicString& id = el.GetIdAttribute();
  if (!id.empty() && HasCaptionMarker(Lower(id)))
    return true;
  const blink::AtomicString& cls = el.GetClassAttribute();
  return !cls.empty() && HasCaptionMarker(Lower(cls));
}

// On the video, visible, not the whole player, with real text.
bool IsCaptionBox(const blink::Element& el,
                  const gfx::Rect& box,
                  const gfx::Rect& vbox) {
  return box.Intersects(vbox) && !box.IsEmpty() &&
         Area(box) <= Area(vbox) * 8 / 10 &&
         !LooksLikeControlText(TextOf(el));
}

}  // namespace

std::string DetectPlayerFamily(blink::Element& video) {
  struct Marker {
    std::string_view token;
    std::string_view family;
  };
  // More specific markers first.
  static constexpr Marker kMarkers[] = {
      {"html5-video-player", "youtube"},  {"ytp-", "youtube"},
      {"html5-video-container", "youtube"},
      {"jw-", "jwplayer"},                {"video-js", "videojs"},
      {"vjs-", "videojs"},                {"shaka-", "shaka"},
      {"plyr", "plyr"},                   {"vp-video", "vimeo"},
      {"vp-player", "vimeo"},             {"bmpui-", "bitmovin"},
      {"bitmovinplayer", "bitmovin"},     {"playkit-", "kaltura"},
      {"kaltura", "kaltura"},             {"art-video-player", "artplayer"},
      {"dplayer", "dplayer"},             {"mejs__", "mediaelement"},
      {"mejs-", "mediaelement"},          {"theoplayer", "theoplayer"},
      {"flowplayer", "flowplayer"},       {"clappr", "clappr"},
      {"wistia", "wistia"},               {"fluid_video", "fluidplayer"},
      {"xgplayer", "xgplayer"},           {"dmp_", "dailymotion"},
      {"brightcove", "brightcove"},       {"vidstack", "vidstack"},
      {"media-player", "vidstack"},       {"mux-player", "mux"},
      {"media-controller", "media-chrome"}, {"amp-video", "amp"},
      {"twitch", "twitch"},               {"smp-toucan", "bbc-smp"},
      {"smpvideoelement", "bbc-smp"},     {"podjav", "podjav"},
      {"videoplayer", "generic-videoplayer"},
      {"video-player", "generic-videoplayer"}, {"player", "generic-player"},
  };
  blink::Element* cur = &video;
  for (int depth = 0; cur && depth < 12; ++depth, cur = cur->parentElement()) {
    const std::string tag = Lower(cur->localName());
    const std::string id = Lower(cur->GetIdAttribute());
    const std::string cls = Lower(cur->GetClassAttribute());
    for (const Marker& m : kMarkers) {
      if (tag.find(m.token) != std::string::npos ||
          id.find(m.token) != std::string::npos ||
          cls.find(m.token) != std::string::npos) {
        return std::string(m.family);
      }
    }
  }
  return "unknown";
}

class SubtitleTextObserver final : public blink::MutationObserver::Delegate {
 public:
  SubtitleTextObserver(blink::Element& root, blink::HTMLVideoElement& video)
      : root_(root), video_(video), start_(base::TimeTicks::Now()) {}
  ~SubtitleTextObserver() override = default;

  blink::ExecutionContext* GetExecutionContext() const override {
    return root_->GetExecutionContext();
  }

  void Trace(blink::Visitor* visitor) const override {
    visitor->Trace(root_);
    visitor->Trace(video_);
    blink::MutationObserver::Delegate::Trace(visitor);
  }

  void Deliver(const blink::MutationRecordVector& records,
               blink::MutationObserver& observer) override {
    records_ += static_cast<int>(records.size());
    if (records_ > kMaxRecordsTotal) {
      observer.disconnect();
      stop_reason_ = "budget";
      return;
    }
    if (base::TimeTicks::Now() - start_ > kObserveFor) {
      observer.disconnect();
      stop_reason_ = "timebox";
      return;
    }

    // For childList take the added nodes, not the target: YouTube swaps the
    // whole cue box under a player-sized container.
    blink::HeapVector<blink::Member<blink::Element>> elements;
    auto add = [&](blink::Node* node) {
      blink::Element* el = blink::DynamicTo<blink::Element>(node);
      if (!el && node)
        el = node->parentElement();
      if (el && !elements.Contains(el))
        elements.push_back(el);
    };
    for (const auto& record : records) {
      if (record->type() == "childList") {
        blink::StaticNodeList* added = record->addedNodes();
        for (unsigned i = 0; added && i < added->length(); ++i)
          add(added->item(i));
      } else {
        add(record->target());
      }
    }

    // One delivery is one cue update, however many nested nodes it touched.
    bool counted = false;
    const gfx::Rect vbox = VideoArea(*video_, *root_);
    int examined = 0, candidates = 0;
    for (blink::Element* el : elements) {
      if (++examined > kMaxElementsPerDelivery ||
          candidates >= kMaxCandidatesPerDelivery) {
        break;
      }
      if (!el->isConnected() ||
          video_->IsShadowIncludingInclusiveAncestorOf(*el)) {
        continue;
      }
      const gfx::Rect box = el->BoundsInWidget();
      if (!box.Intersects(vbox) || Area(box) > Area(vbox) * 8 / 10)
        continue;
      const std::string text = TextOf(*el);
      if (LooksLikeControlText(text) || text == last_text_)
        continue;
      ++candidates;
      last_text_ = text;
      if (!counted) {
        counted = true;
        ++changes_;
        if (ElementHasCaptionMarker(*el))
          ++marker_changes_;
      }
    }
  }

  int changes() const { return changes_; }
  int marker_changes() const { return marker_changes_; }
  const char* stop_reason() const { return stop_reason_; }

 private:
  blink::Member<blink::Element> root_;
  blink::Member<blink::HTMLVideoElement> video_;
  const base::TimeTicks start_;
  std::string last_text_;
  int records_ = 0;
  int changes_ = 0;
  int marker_changes_ = 0;
  const char* stop_reason_ = nullptr;
};

SubtitleProbe* SubtitleProbe::GetInstance() {
  static base::NoDestructor<SubtitleProbe> instance;
  return instance.get();
}

SubtitleProbe::SubtitleProbe() = default;
SubtitleProbe::~SubtitleProbe() = default;

blink::Element* SubtitleProbe::PlayerRoot(blink::Element& requested,
                                          blink::HTMLVideoElement& video) {
  if (&requested != &video)
    return &requested;
  // video.requestFullscreen(): climb while the ancestor is about video-sized.
  const int64_t video_area = Area(video.BoundsInWidget());
  blink::Element* root = &video;
  blink::Element* cur = video.parentElement();
  for (int depth = 0; cur && depth < 6; ++depth, cur = cur->parentElement()) {
    if (Area(cur->BoundsInWidget()) > video_area * 3 / 2 + 1)
      break;
    root = cur;
  }
  return root;
}

void SubtitleProbe::ScanOnce(blink::Element& root,
                             blink::HTMLVideoElement& video) {
  const gfx::Rect vbox = VideoArea(video, root);
  int elements = 0;

  // Root subtree, shadow roots included; the <video> subtree (UA cue box) is
  // skipped.
  auto visit = [&](auto& self, blink::ContainerNode& scope) -> void {
    for (blink::Node* n = blink::NodeTraversal::Next(scope, &scope); n;) {
      if (++elements > kMaxScannedElements)
        return;
      blink::Element* el = blink::DynamicTo<blink::Element>(n);
      if (!el || el == &video) {
        n = blink::NodeTraversal::NextSkippingChildren(*n, &scope);
        continue;
      }
      if (ElementHasCaptionMarker(*el) &&
          IsCaptionBox(*el, el->BoundsInWidget(), vbox)) {
        ++marker_hits_;
      }
      if (blink::ShadowRoot* shadow = el->GetShadowRoot()) {
        if (shadow_roots_.size() < kMaxShadowRoots)
          shadow_roots_.emplace_back(shadow);
        self(self, *shadow);
      }
      n = blink::NodeTraversal::Next(*n, &scope);
    }
  };
  if (blink::ShadowRoot* shadow = root.GetShadowRoot()) {
    shadow_roots_.emplace_back(shadow);
    visit(visit, *shadow);
  }
  visit(visit, root);
  if (marker_hits_ > 0)
    return;

  // Caption layer outside the root (JW-style embeds): marker elements over
  // the video anywhere in the document, their parents get observed too.
  blink::Element* doc_root = root.GetDocument().documentElement();
  int doc_elements = 0;
  for (blink::Node* n =
           doc_root ? blink::NodeTraversal::Next(*doc_root, doc_root) : nullptr;
       n;) {
    if (++doc_elements > kMaxDocumentElements)
      break;
    blink::Element* el = blink::DynamicTo<blink::Element>(n);
    if (!el || el == &root || el == &video) {
      n = blink::NodeTraversal::NextSkippingChildren(*n, doc_root);
      continue;
    }
    if (ElementHasCaptionMarker(*el)) {
      const gfx::Rect box = el->BoundsInWidget();
      if (box.Intersects(vbox) && !box.IsEmpty()) {
        if (IsCaptionBox(*el, box, vbox))
          ++marker_hits_;
        if (extra_targets_.size() < kMaxExtraTargets)
          extra_targets_.emplace_back(el->parentElement() ? el->parentElement()
                                                          : el);
      }
    }
    n = blink::NodeTraversal::Next(*n, doc_root);
  }
}

void SubtitleProbe::OnEnterFullscreen(blink::Element& requested,
                                      blink::HTMLVideoElement& video) {
  Stop("restart");

  const std::string family = DetectPlayerFamily(video);
  video_ = &video;
  shadow_roots_.clear();
  extra_targets_.clear();
  marker_hits_ = 0;
  session_start_ = base::TimeTicks::Now();
  player_known_ = family != "unknown" && !family.starts_with("generic");
  video_box_empty_ = video.BoundsInWidget().IsEmpty();

  blink::Element* root = PlayerRoot(requested, video);
  ScanOnce(*root, video);

  delegate_ = blink::MakeGarbageCollected<SubtitleTextObserver>(*root, video);
  observer_ = blink::MutationObserver::Create(delegate_.Get());
  blink::MutationObserverInit* init = blink::MutationObserverInit::Create();
  init->setChildList(true);
  init->setCharacterData(true);
  init->setSubtree(true);
  observer_->observe(root, init, ASSERT_NO_EXCEPTION);
  for (auto& shadow : shadow_roots_)
    observer_->observe(shadow.Get(), init, ASSERT_NO_EXCEPTION);
  for (auto& extra : extra_targets_)
    observer_->observe(extra.Get(), init, ASSERT_NO_EXCEPTION);
}

void SubtitleProbe::Stop(const char* reason,
                         blink::Element* fullscreen_element) {
  if (!observer_)
    return;
  // The element may be the <video> or a container around it.
  if (fullscreen_element &&
      !fullscreen_element->IsShadowIncludingInclusiveAncestorOf(*video_)) {
    return;
  }
  observer_->disconnect();
  SendReport(reason, delegate_.Get());
  observer_.Clear();
  delegate_.Clear();
  video_.Clear();
  shadow_roots_.clear();
  extra_targets_.clear();
}

void SubtitleProbe::SendReport(const char* reason,
                               SubtitleTextObserver* delegate) {
  // A video that left its document is a session we never saw end.
  if (!video_ || !video_->isConnected())
    return;
  const int32_t duration_s = static_cast<int32_t>(
      (base::TimeTicks::Now() - session_start_).InSeconds());
  // JW Player requests fullscreen twice; the 0 s first session is not data.
  if (duration_s == 0 && std::string_view(reason) != "exit")
    return;

  auto report = media::mojom::blink::SubtitleProbeReport::New();
  const int changes = delegate ? delegate->changes() : 0;
  const int marker_changes = delegate ? delegate->marker_changes() : 0;
  if (marker_changes > 0) {
    report->dom_candidate = "marker";
  } else if (changes >= 2) {
    report->dom_candidate = "heuristic";
  } else if (marker_hits_ > 0) {
    report->dom_candidate = "marker-static";  // e.g. a "Subtitles" menu item
  } else {
    report->dom_candidate = "none";
  }
  report->dom_changes = changes;
  report->dom_marker_hits = marker_hits_;
  report->player_known = player_known_;
  report->video_box_empty = video_box_empty_;
  report->observer_stop = delegate && delegate->stop_reason()
                              ? delegate->stop_reason()
                              : std::string_view(reason) == "exit" ? "exit"
                                                                    : "reset";
  report->duration_s = duration_s;
  video_->SendAlohaSubtitleReport(std::move(report));
}

}  // namespace aloha
