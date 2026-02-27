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

#ifndef COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_SERVICE_H_

#include "components/adblock/core/snippet/snippet_service.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace adblock {

class MockSnippetService : public SnippetService {
 public:
  MockSnippetService();
  ~MockSnippetService() override;

  MOCK_METHOD(std::string,
              GenerateSnippetScript,
              (const GURL&, base::ListValue),
              (override));
  MOCK_METHOD(std::string, GetSnippetVersion, (), (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_SERVICE_H_
