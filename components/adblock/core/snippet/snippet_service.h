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

#ifndef COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_SERVICE_H_

#include <string>

#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;

namespace adblock {

class SnippetService : public KeyedService {
 public:
  virtual std::string GenerateSnippetScript(const GURL& url,
                                            base::ListValue input) = 0;
  virtual std::string GetSnippetVersion() = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_SERVICE_H_
