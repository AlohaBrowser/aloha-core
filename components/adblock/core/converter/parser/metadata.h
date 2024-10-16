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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_METADATA_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_METADATA_H_

#include <istream>
#include <string>

#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace adblock {

class Metadata {
 public:
  static absl::optional<Metadata> FromStream(std::istream& filter_stream);
  static Metadata Default();

  Metadata(const Metadata& other);
  Metadata(Metadata&& other);
  ~Metadata();

  const std::string homepage;
  const std::string title;
  const std::string version;
  const absl::optional<GURL> redirect_url;
  const base::TimeDelta expires;

  static constexpr base::TimeDelta kDefaultExpirationInterval = base::Days(5);
  static constexpr base::TimeDelta kMaxExpirationInterval = base::Days(14);
  static constexpr base::TimeDelta kMinExpirationInterval = base::Hours(1);

 private:
  Metadata(std::string homepage,
           std::string title,
           std::string version,
           absl::optional<GURL> redirect_url,
           base::TimeDelta expires);
  Metadata();

  static bool IsValidAdblockHeader(const std::string& adblock_header);
  static base::TimeDelta ParseExpirationTime(
      const std::string& expiration_value);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_PARSER_METADATA_H_
