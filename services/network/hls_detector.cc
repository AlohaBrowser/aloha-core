// Copyright 2026 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "hls_detector.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"

#include "base/files/file_util.h"
#include "base/path_service.h"

// ALOHA: https://app.clickup.com/t/86ewr2k2q 
namespace network {
    
    HLSDetector::DetectionResult::DetectionResult()
        : is_hls(false), confidence(DetectionConfidence::NONE) {}
    
    HLSDetector::DetectionResult::DetectionResult(const DetectionResult& other)
        : is_hls(other.is_hls),
          confidence(other.confidence),
          method(other.method),
          playlist_url(other.playlist_url),
          content(other.content) {}
    
    HLSDetector::DetectionResult& HLSDetector::DetectionResult::operator=(
        const DetectionResult& other) {
        if (this != &other) {
            is_hls = other.is_hls;
            confidence = other.confidence;
            method = other.method;
            playlist_url = other.playlist_url;
            content = other.content;
        }
        return *this;
    }
    HLSDetector::DetectionResult::~DetectionResult() = default;
    
    HLSDetector::HLSDetector() = default;
    HLSDetector::~HLSDetector() = default;
    
    HLSDetector::DetectionResult HLSDetector::CheckHeaders(net::URLRequest* request) {
        DetectionResult result;
        
        const GURL& url = request->url();
        std::string url_string = url.spec();
        std::string path(url.path());
        
        // 1. Content-Type (high confidence)
        if (auto* headers = request->response_headers()) {
            std::string content_type;
            auto ct_opt = headers->GetNormalizedHeader("Content-Type");
            if (ct_opt.has_value()) content_type = base::ToLowerASCII(ct_opt.value());
            
            // All known MIME types for HLS
            static const char* kHLSMimeTypes[] = {
                "application/vnd.apple.mpegurl",
                "application/x-mpegurl",
                "audio/mpegurl",
                "audio/x-mpegurl",
                "application/mpegurl",
                "vnd.apple.mpegurl"
            };
            
            for (const char* mime : kHLSMimeTypes) {
                if (content_type.find(mime) != std::string::npos) {
                    result.is_hls = true;
                    result.confidence = DetectionConfidence::MEDIUM;
                    result.method = "content-type: " + std::string(mime);
                    result.playlist_url = url_string;
                    return result;
                }
            }
        }
        
        // 2. File extension (low confidence)
        path = base::ToLowerASCII(path);
        if (path.find(".m3u8") != std::string::npos || 
            path.find(".m3u") != std::string::npos) {
            result.is_hls = true;
            result.confidence = DetectionConfidence::LOW;
            result.method = "extension";
            result.playlist_url = url_string;
            return result;
        }
        
        // 3. Heuristic: suspicious patterns in URL
        if (IsLikelyHLSByURL(url_string)) {
            result.is_hls = true;
            result.confidence = DetectionConfidence::LOW;
            result.method = "url-pattern";
            result.playlist_url = url_string;
            pending_checks_[url_string] = true;  // Check content
            return result;
        }
        
        // 4. Check for video/* with small size (might be a manifest)
        if (auto* headers = request->response_headers()) {
            std::string content_type;
            auto ct_opt = headers->GetNormalizedHeader("Content-Type");
            if (ct_opt.has_value()) content_type = ct_opt.value();
            
            int64_t content_length = request->GetExpectedContentSize();
            
            // Video content, but small size = might be a manifest
            if (content_type.find("video/") != std::string::npos &&
                content_length > 0 && content_length < 100 * 1024) {
                pending_checks_[url_string] = true;
                result.method = "suspicious-video-size";
            }
        }
        
        return result;
    }

    // Stage 2: Check content (call in DidRead)
    HLSDetector::DetectionResult HLSDetector::CheckContent(const std::string& url, 
                                 const char* data, 
                                 size_t size) {
        DetectionResult result;
        
        // Need at least one fragment for checking
        if (size < 7) return result;  // Minimum "#EXTM3U"
        
        std::string content(data, std::min(size, size_t(10000)));
        bool is_hls = false;
        
        // HLS manifest ALWAYS starts with #EXTM3U
        if (content.compare(0, 7, "#EXTM3U") == 0) {
            is_hls = true;
            result.confidence = DetectionConfidence::HIGH;
            result.method = "content-header";
            result.content = content; // Save for passing to Java
            // Additional check for HLS-specific tags
            if (content.find("#EXT-X-") != std::string::npos) {
                result.method = "content-hls-tags";
            }
        }
        
        // Alternative: check for HLS tags somewhere in the content
        // (in case there is a BOM or spaces at the beginning)
        if (!is_hls && content.find("#EXTM3U") != std::string::npos &&
            content.find("#EXT") != std::string::npos) {
            is_hls = true;
            result.confidence = DetectionConfidence::HIGH;
            result.method = "content-tags-present";
            result.content = content; // Save for passing to Java
        }

        if (is_hls) {
            result.is_hls = true;
            result.playlist_url = url; // By default - current URL

            // Attempt to extract a child link (for Master Playlist)
            // Look for .m3u8 after tags like #EXT-X-STREAM-INF
            size_t pos = content.find("#EXT-X-STREAM-INF");
            if (pos != std::string::npos) {
                size_t line_end = content.find("\n", pos);
                if (line_end != std::string::npos) {
                    size_t link_start = content.find_first_not_of(" \r\n", line_end);
                    if (link_start != std::string::npos) {
                        size_t link_end = content.find_first_of(" \r\n", link_start);
                        std::string sub_link = content.substr(link_start, link_end - link_start);
                        result.content = sub_link; // Save for passing to Java
                        if (sub_link.find(".m3u8") != std::string::npos) {
                            // If it's a relative path - combine with the base URL
                            GURL base_url(url);
                            GURL resolved_url = base_url.Resolve(sub_link);
                            if (resolved_url.is_valid()) {
                                result.playlist_url = resolved_url.spec();
                                result.method += "+master-link";
                            }
                        }
                    }
                }
            }
            pending_checks_.erase(url);
        } else {
            pending_checks_.erase(url);
        }
        
        return result;
    }

    bool HLSDetector::IsLikelyHLSByURL(const std::string& url) {
        // base::StringPiece url_piece(url); // Removed, not used
        
        // Common HLS URL patterns
        static const char* kHLSPatterns[] = {
            "/hls/",
            "/playlist",
            "/manifest",
            "m3u8",
            "stream.m3u",
            "/live/",
            "/vod/",
            "index_",  // index_0_av.m3u8, index_1_av.m3u8
            "chunklist",
            "media_",
        };
        
        std::string lower_url = base::ToLowerASCII(url);        
        
        for (const char* pattern : kHLSPatterns) {
            if (lower_url.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Query parameter patterns
        if (lower_url.find("format=m3u8") != std::string::npos ||
            lower_url.find(".m3u8?") != std::string::npos) {
            return true;
        }
        
        return false;
    }
}