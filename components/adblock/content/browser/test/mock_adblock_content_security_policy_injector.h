/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_ADBLOCK_CONTENT_SECURITY_POLICY_INJECTOR_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_ADBLOCK_CONTENT_SECURITY_POLICY_INJECTOR_H_

#include "components/adblock/content/browser/content_security_policy_injector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockAdblockContentSecurityPolicyInjector
    : public ContentSecurityPolicyInjector {
 public:
  MockAdblockContentSecurityPolicyInjector();
  ~MockAdblockContentSecurityPolicyInjector() override;

  MOCK_METHOD(void,
              InsertContentSecurityPolicyHeadersIfApplicable,
              (const GURL&,
               content::GlobalRenderFrameHostId,
               const scoped_refptr<net::HttpResponseHeaders>&,
               InsertContentSecurityPolicyHeadersCallback),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_ADBLOCK_CONTENT_SECURITY_POLICY_INJECTOR_H_
