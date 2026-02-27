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

#pragma once

#include "net/url_request/url_request.h"
#include <string>
#include <map>


// ALOHA: https://app.clickup.com/t/86ewr2k2q 
namespace network {
  class HLSDetector {
    public:
        enum class DetectionConfidence {
            NONE,
            LOW,      //  URL
            MEDIUM,   // Content-Type
            HIGH      // Content inside the manifest
        };
        
        struct DetectionResult {
            bool is_hls;
            DetectionConfidence confidence;
            std::string method;
            std::string playlist_url;
            std::string content;
              
            DetectionResult();
            DetectionResult(const DetectionResult&);
            DetectionResult& operator=(const DetectionResult&);
            ~DetectionResult();
        };

        DetectionResult CheckHeaders(net::URLRequest* request);
          HLSDetector();
          ~HLSDetector();
        DetectionResult CheckContent(const std::string& url, 
                                 const char* data, 
                                 size_t size);        
    private:

      bool IsLikelyHLSByURL(const std::string& url);

      // Cache of suspicious URLs for content checking
      std::map<std::string, bool> pending_checks_;
      base::WeakPtrFactory<HLSDetector> weak_ptr_factory_{this};
  };
}