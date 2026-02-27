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
// Classifies HTTP responses as subtitle files by Content-Type, extension and
// body magic; confirmed categories go to the browser through the reporter.

#ifndef SERVICES_NETWORK_SUBTITLE_DETECTOR_H_
#define SERVICES_NETWORK_SUBTITLE_DETECTOR_H_

#include <string>
#include <string_view>

#include "base/functional/callback.h"

namespace net {
class URLRequest;
}

namespace network {

class SubtitleDetector {
 public:
  // Category names: "vtt", "srt", "ttml", "ass", "sbv", "json-captions", ...
  using Reporter = base::RepeatingCallback<void(const std::string& kind)>;

  SubtitleDetector();
  SubtitleDetector(const SubtitleDetector&) = delete;
  SubtitleDetector& operator=(const SubtitleDetector&) = delete;
  ~SubtitleDetector();

  void SetReporter(Reporter reporter) { reporter_ = std::move(reporter); }

  void OnResponseStarted(const net::URLRequest& request);
  // First body bytes, at most net::kMaxBytesToSniff; called at most once.
  void OnFirstChunk(std::string_view data);
  // Reports a header-conclusive kind whose body never arrived (304, 204).
  void OnComplete();

 private:
  enum class Kind {
    kNone,
    kVtt,
    kSrt,
    kTtml,
    kAss,
    kSbv,
    kMicroDvd,
    kLrc,
    kJsonCaptions,
    kGeneric,  // URL hint only; needs the body to confirm
  };

  static const char* KindName(Kind kind);
  static Kind KindFromExtension(std::string_view path);
  static Kind KindFromContentType(std::string_view content_type);
  static const char* UrlHint(std::string_view url);
  static Kind KindFromMagic(std::string_view body);

  void Report(std::string_view kind) const;

  Reporter reporter_;
  std::string url_;
  std::string content_type_;
  Kind header_kind_ = Kind::kNone;
  bool sniff_pending_ = false;
};

}  // namespace network

#endif  // SERVICES_NETWORK_SUBTITLE_DETECTOR_H_
