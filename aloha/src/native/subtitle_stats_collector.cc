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

#include "aloha/src/native/subtitle_stats_collector.h"

#include <vector>

#include "content/public/browser/navigation_handle.h"

namespace aloha {

SubtitleStatsCollector::SubtitleStatsCollector(
    content::WebContents* web_contents)
    : content::WebContentsUserData<SubtitleStatsCollector>(*web_contents),
      content::WebContentsObserver(web_contents) {}

SubtitleStatsCollector::~SubtitleStatsCollector() = default;

void SubtitleStatsCollector::OnNetworkKind(const std::string& kind) {
  if (kind == "hls-has-text-tracks") {
    hls_text_ = true;
  } else if (kind == "hls-subtitle-media-playlist") {
    hls_subtitle_playlist_ = true;  // HLS subtitle segments are WebVTT
    net_formats_.insert("vtt");
  } else if (kind != "hls-no-text-tracks" && kind != "hls-media-playlist") {
    net_formats_.insert(kind);
  }
}

void SubtitleStatsCollector::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Same-document navigations included: on SPA video sites each video is one.
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }
  net_formats_.clear();
  hls_text_ = hls_subtitle_playlist_ = false;
}

std::string SubtitleStatsCollector::OnRendererReport(
    const media::mojom::SubtitleProbeReport& report) {
  const bool page_dom =
      report.dom_changes > 0 && (report.dom_candidate == "marker" ||
                                 report.dom_candidate == "heuristic");
  const bool tracks_usable =
      report.text_tracks_count > 0 && report.text_tracks_renderable > 0;
  const bool native_hls = hls_text_ && report.src_kind == "url";

  // Precedence: works (1), cheapest fix first (5, 2/4, 3), then 6, 0.
  char code = '0';
  std::vector<std::string> tokens;
  if (report.engine_rendering) {
    code = '1';
    if (report.forced_track)
      tokens.push_back("forced");
    if (report.dom_changes > 0)
      tokens.push_back("dual");
  } else if (tracks_usable) {
    code = '5';
  } else if (page_dom) {
    code = report.player_known ? '2' : '4';
    tokens.push_back(report.player_known ? "known" : "unk");
  } else if (hls_text_ && !hls_subtitle_playlist_) {
    // Advertised but never fetched: the player (or the native engine) left
    // the text renditions alone.
    code = '3';
    tokens.push_back(native_hls ? "native" : "mse");
  } else if (!net_formats_.empty()) {
    code = '6';  // fetched (a subtitle playlist counts), nothing shown
  }

  const bool low_confidence =
      report.duration_s < 10 ||
      (page_dom && !report.engine_rendering && report.dom_changes < 2) ||
      report.observer_stop == "budget";
  if (low_confidence)
    tokens.push_back("low");

  for (const char* f : {"vtt", "srt", "ttml", "ass", "sbv", "json-captions"}) {
    if (net_formats_.count(f))
      tokens.push_back(std::string_view(f) == "json-captions" ? "json" : f);
  }
  if (report.src_kind == "blob")
    tokens.push_back("blob");
  else if (report.src_kind == "url")
    tokens.push_back("url");
  else if (report.src_kind == "mediastream")
    tokens.push_back("ms");

  // Message: tokens in priority order while they fit in 15 characters.
  constexpr size_t kMaxMessage = 15;
  std::string message;
  for (const std::string& token : tokens) {
    const size_t next = message.empty() ? token.size()
                                        : message.size() + 1 + token.size();
    if (next > kMaxMessage)
      continue;
    if (!message.empty())
      message += ' ';
    message += token;
  }
  std::string stat(1, code);
  if (!message.empty())
    stat += " " + message;
  return stat;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SubtitleStatsCollector);

}  // namespace aloha
