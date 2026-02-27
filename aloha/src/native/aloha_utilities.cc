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

#include "aloha_utilities.h"
#include "base/no_destructor.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace aloha {

    namespace {
        constexpr char kGoogleAuthPattern[] =
            "https://accounts.google.com/o/oauth2/auth/*";
    }

    // https://app.clickup.com/t/86ev1nrnd
    // Parse whether Google authentication requires third-party cookies for |target_url|.
    bool IsGoogleAuthRequire3PCookies(const GURL& initiator_url, const GURL& target_url) {
        static const base::NoDestructor<GURL> kGoogleAuthURL(kGoogleAuthPattern);

        return initiator_url.SchemeIsHTTPOrHTTPS() &&
                target_url.SchemeIsHTTPOrHTTPS() &&
                net::registry_controlled_domains::SameDomainOrHost(
                target_url, *kGoogleAuthURL,
                net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) &&
                target_url.has_query() &&
                target_url.query().find("redirect_uri=storagerelay") != std::string::npos;
    }
}