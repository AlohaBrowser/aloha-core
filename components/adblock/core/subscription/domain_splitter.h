
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_

#include <string_view>

#include "absl/types/optional.h"

namespace adblock {

// When constructed with a full domain like "aaa.bbb.ccc.com", subsequent calls
// to FindNextSubdomain() will yield "aaa.bbb.ccc.com", "bbb.ccc.com",
// "ccc.com", "com" and then versions with the TLD removed, for matching
// wildcard filters: "aaa.bbb.ccc." then "bbb.ccc." then "ccc.". A nullopt is
// returned when the end of the domain is reached.
class DomainSplitter {
 public:
  // |domain| must outlive this, no copy made.
  explicit DomainSplitter(std::string_view domain);
  // Returns reference to part of |domain|.
  absl::optional<std::string_view> FindNextSubdomain();

 private:
  std::string_view& GetNonEmptyRemainingDomain();
  std::string_view remaining_domain_;
  // HACK - the copy of the string is needed to null-terminate the
  // "remaining_domain_sans_registry_" member. We need a null-terminated source
  // string even though string_view has a size() because of
  // https://github.com/google/flatbuffers/issues/8200.
  // TODO: remove this hack when flatbuffers is updated -
  // remaining_domain_sans_registry_ could refer to the string_view passed in
  // ctor instead and avoid any allocations.
  std::string domain_sans_registry_copy_;
  std::string_view remaining_domain_sans_registry_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_
