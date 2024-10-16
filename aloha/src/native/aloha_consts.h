// Copyright 2024 Aloha Mobile Ltd.

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

namespace aloha {
    // X-Requested-With
    constexpr const char *kAlohaBrowser = "AlohaBrowser";

    // Cookies
    constexpr int kCookieManagersCount = 2;
    constexpr int kDefaultCookieManager = 0;
    constexpr int kNormalCookieManager = 0;
    constexpr int kPrivateCookieManager = 1;

    // Scheme
    constexpr const char *kAlohaScheme = "aloha";

    extern char g_callstack_file_name[1024];

    // ALOHA https://app.clickup.com/t/2u59j0h
    constexpr char kDownloadRequestOrigin[] = "Aloha download";
    // See comment in android_webview/browser/network_service/net_helpers.cc::GetHttpCacheSize()
    constexpr int kHttpCacheMaxSizeBytes = 150 * 1024 * 1024;
    // The same constant in net/disk_cache/simple/simple_backend_impl.cc
    constexpr int kHttpCacheFileRatio = 8;
    constexpr int kHttpCacheMaxFileSizeBytes = kHttpCacheMaxSizeBytes / kHttpCacheFileRatio;
}
