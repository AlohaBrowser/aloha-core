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
// Per-tab merge of network subtitle categories with the renderer's
// end-of-fullscreen report into one analytics line.

#ifndef ALOHA_SRC_NATIVE_SUBTITLE_STATS_COLLECTOR_H_
#define ALOHA_SRC_NATIVE_SUBTITLE_STATS_COLLECTOR_H_

#include <set>
#include <string>

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "media/mojo/mojom/media_player.mojom.h"

namespace aloha {

class SubtitleStatsCollector
    : public content::WebContentsUserData<SubtitleStatsCollector>,
      public content::WebContentsObserver {
 public:
  SubtitleStatsCollector(const SubtitleStatsCollector&) = delete;
  SubtitleStatsCollector& operator=(const SubtitleStatsCollector&) = delete;
  ~SubtitleStatsCollector() override;

  // Subtitle format ("vtt", "json-captions", ...) or HLS manifest verdict
  // ("hls-has-text-tracks", ...). Kept until the next primary navigation.
  void OnNetworkKind(const std::string& kind);

  // Returns "<code> <message>": see BromiumClient.onSubtitleStat().
  std::string OnRendererReport(const media::mojom::SubtitleProbeReport& report);

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend class content::WebContentsUserData<SubtitleStatsCollector>;
  explicit SubtitleStatsCollector(content::WebContents* web_contents);

  std::set<std::string> net_formats_;
  bool hls_text_ = false;            // multivariant playlist advertises text
  bool hls_subtitle_playlist_ = false;  // a subtitle media playlist was fetched

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace aloha

#endif  // ALOHA_SRC_NATIVE_SUBTITLE_STATS_COLLECTOR_H_
