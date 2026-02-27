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

#include "services/network/subtitle_detector.h"

#include <string_view>

#include "base/strings/string_util.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace network {

namespace {

std::string_view SkipBomAndSpace(std::string_view body) {
  if (body.starts_with("\xEF\xBB\xBF")) {
    body.remove_prefix(3);
  }
  while (!body.empty() && base::IsAsciiWhitespace(body.front())) {
    body.remove_prefix(1);
  }
  return body;
}

// Never a subtitle file whatever the URL says (a player's captions.js).
bool IsNeverSubtitleType(std::string_view content_type) {
  return content_type.find("javascript") != std::string_view::npos ||
         content_type.find("ecmascript") != std::string_view::npos ||
         content_type.starts_with("text/css") ||
         content_type.starts_with("image/") ||
         content_type.starts_with("font/") ||
         content_type.starts_with("video/") ||
         content_type.starts_with("audio/") ||
         content_type.starts_with("application/wasm") ||
         content_type.starts_with("application/font");
}

// Types subtitle files are commonly served under; worth sniffing the body.
bool IsOpaqueTextType(std::string_view content_type) {
  return content_type.empty() || content_type.starts_with("text/plain") ||
         content_type.starts_with("application/octet-stream") ||
         content_type.starts_with("text/xml") ||
         content_type.starts_with("application/xml") ||
         content_type.starts_with("text/html");
}

}  // namespace

SubtitleDetector::SubtitleDetector() = default;
SubtitleDetector::~SubtitleDetector() = default;

// static
const char* SubtitleDetector::KindName(Kind kind) {
  switch (kind) {
    case Kind::kNone:
      return "none";
    case Kind::kVtt:
      return "vtt";
    case Kind::kSrt:
      return "srt";
    case Kind::kTtml:
      return "ttml";
    case Kind::kAss:
      return "ass";
    case Kind::kSbv:
      return "sbv";
    case Kind::kMicroDvd:
      return "microdvd";
    case Kind::kLrc:
      return "lrc";
    case Kind::kJsonCaptions:
      return "json-captions";
    case Kind::kGeneric:
      return "generic";
  }
  return "?";
}

// static
SubtitleDetector::Kind SubtitleDetector::KindFromExtension(
    std::string_view path) {
  struct Entry {
    std::string_view ext;
    Kind kind;
  };
  static constexpr Entry kTable[] = {
      {".vtt", Kind::kVtt},      {".webvtt", Kind::kVtt},
      {".srt", Kind::kSrt},      {".ttml", Kind::kTtml},
      {".dfxp", Kind::kTtml},    {".ttml2", Kind::kTtml},
      {".ass", Kind::kAss},      {".ssa", Kind::kAss},
      {".sbv", Kind::kSbv},      {".sub", Kind::kMicroDvd},
      {".lrc", Kind::kLrc},
  };
  for (const Entry& e : kTable) {
    if (path.ends_with(e.ext)) {
      return e.kind;
    }
  }
  return Kind::kNone;
}

// static
SubtitleDetector::Kind SubtitleDetector::KindFromContentType(
    std::string_view content_type) {
  struct Entry {
    std::string_view mime;
    Kind kind;
  };
  static constexpr Entry kTable[] = {
      {"text/vtt", Kind::kVtt},
      {"text/webvtt", Kind::kVtt},
      {"application/x-subrip", Kind::kSrt},
      {"application/srt", Kind::kSrt},
      {"text/srt", Kind::kSrt},
      {"application/ttml+xml", Kind::kTtml},
      {"application/ttaf+xml", Kind::kTtml},
      {"text/x-ssa", Kind::kAss},
      {"application/x-ass", Kind::kAss},
      {"application/x-subtitle", Kind::kGeneric},
  };
  for (const Entry& e : kTable) {
    if (content_type.starts_with(e.mime)) {
      return e.kind;
    }
  }
  return Kind::kNone;
}

// static
const char* SubtitleDetector::UrlHint(std::string_view url) {
  // Captions served as JSON or without an extension (YouTube, Vimeo, ...).
  static constexpr std::string_view kHints[] = {
      "timedtext",   "/texttrack", "/captions",  "/caption/", "/subtitles",
      "/subtitle/",  "/subs/",     "webvtt",     "caption=",  "subtitle=",
      "/cc/",
  };
  for (std::string_view hint : kHints) {
    if (url.find(hint) != std::string_view::npos) {
      return hint.data();
    }
  }
  return nullptr;
}

// static
SubtitleDetector::Kind SubtitleDetector::KindFromMagic(std::string_view body) {
  body = SkipBomAndSpace(body);
  if (body.starts_with("WEBVTT")) {
    return Kind::kVtt;
  }
  if (body.starts_with("[Script Info]")) {
    return Kind::kAss;
  }
  if (body.find("http://www.w3.org/ns/ttml") != std::string_view::npos ||
      body.find("<tt ") != std::string_view::npos ||
      body.find("<tt:tt") != std::string_view::npos) {
    return Kind::kTtml;
  }
  // SRT: cue counter, then "HH:MM:SS,mmm --> HH:MM:SS,mmm".
  if (!body.empty() && base::IsAsciiDigit(body.front())) {
    std::string_view head = body.substr(0, 64);
    if (head.find("-->") != std::string_view::npos &&
        head.find(',') != std::string_view::npos) {
      return Kind::kSrt;
    }
  }
  // SBV: "0:00:00.000,0:00:02.000".
  if (body.size() > 12 && base::IsAsciiDigit(body.front()) &&
      body[1] == ':' && body.substr(0, 32).find(',') != std::string_view::npos &&
      body.substr(0, 32).find("-->") == std::string_view::npos &&
      body.substr(0, 32).find('.') != std::string_view::npos) {
    return Kind::kSbv;
  }
  // YouTube timedtext JSON3 / srv3.
  if (body.starts_with("{") &&
      (body.find("\"tStartMs\"") != std::string_view::npos ||
       body.find("\"wireMagic\"") != std::string_view::npos)) {
    return Kind::kJsonCaptions;
  }
  if (body.starts_with("<timedtext") || body.starts_with("<transcript")) {
    return Kind::kJsonCaptions;
  }
  return Kind::kNone;
}

void SubtitleDetector::Report(std::string_view kind) const {
  if (reporter_) {
    reporter_.Run(std::string(kind));
  }
}

void SubtitleDetector::OnResponseStarted(const net::URLRequest& request) {
  const GURL& url = request.url();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  url_ = url.spec();
  std::string path = base::ToLowerASCII(url.path());

  if (const net::HttpResponseHeaders* headers = request.response_headers()) {
    std::optional<std::string> ct = headers->GetNormalizedHeader("Content-Type");
    if (ct) {
      content_type_ = base::ToLowerASCII(*ct);
    }
  }

  Kind by_ext = KindFromExtension(path);
  Kind by_type = KindFromContentType(content_type_);
  if (by_type != Kind::kNone) {
    header_kind_ = by_type;
  } else if (by_ext != Kind::kNone) {
    header_kind_ = by_ext;
  } else if (!IsNeverSubtitleType(content_type_) &&
             UrlHint(base::ToLowerASCII(url_))) {
    header_kind_ = Kind::kGeneric;
  }

  // Header verdicts still wait for the body: it confirms, and it reveals
  // VTT storyboards.
  sniff_pending_ =
      header_kind_ != Kind::kNone || IsOpaqueTextType(content_type_);
}

void SubtitleDetector::OnComplete() {
  if (!sniff_pending_) {
    return;
  }
  sniff_pending_ = false;
  if (header_kind_ != Kind::kNone && header_kind_ != Kind::kGeneric) {
    Report(KindName(header_kind_));
  }
}

void SubtitleDetector::OnFirstChunk(std::string_view data) {
  if (!sniff_pending_) {
    return;
  }
  sniff_pending_ = false;
  if (data.size() < 8) {
    return;
  }

  Kind magic = KindFromMagic(data);
  // Seek-thumbnail storyboards ("image.jpg#xywh=") are not subtitles.
  if (magic == Kind::kVtt && data.find("#xywh=") != std::string_view::npos) {
    return;
  }
  if (magic != Kind::kNone) {
    Report(KindName(magic));
  } else if (header_kind_ != Kind::kNone && header_kind_ != Kind::kGeneric) {
    Report(KindName(header_kind_));
  }
}

}  // namespace network
