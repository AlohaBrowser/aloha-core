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

#ifndef COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_STORAGE_H_
#define COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_STORAGE_H_

#include "components/adblock/core/snippet/snippet_storage.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace adblock {

class MockSnippetStorage : public SnippetStorage {
 public:
  MockSnippetStorage();
  ~MockSnippetStorage() override;

  MOCK_METHOD(const std::string&, GetLoadedSnippets, (), (const, override));
  MOCK_METHOD(void, LoadSnippets, (const std::string&), (override));
  MOCK_METHOD(void,
              StoreSnippets,
              (const std::string& content,
               base::OnceCallback<void(const std::string&)> callback),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SNIPPET_TEST_MOCK_SNIPPET_STORAGE_H_
