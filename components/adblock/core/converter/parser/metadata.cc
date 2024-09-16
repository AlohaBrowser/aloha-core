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

#include "components/adblock/core/converter/parser/metadata.h"

#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace adblock {

// Parses the stream line by line until it finds comments. After the first non
// comment line any upcoming comments will be skipped.
// static
absl::optional<Metadata> Metadata::FromStream(std::istream& filter_stream) {
  static re2::RE2 comment_re("^!\\s*(.*?)\\s*:\\s*(.*)");

  std::string homepage;
  std::string title;
  std::string version;
  absl::optional<GURL> redirect_url;
  base::TimeDelta expires = kDefaultExpirationInterval;

  std::string line;
  std::getline(filter_stream, line);
  if (!IsValidAdblockHeader(line)) {
    VLOG(1) << "[eyeo] Invalid filter list. Should start with [Adblock Plus "
               "<x>.<y>].";
    return {};
  }

  std::string key, value;
  // Process stream until the line is a comment
  auto position_in_stream = filter_stream.tellg();
  while (std::getline(filter_stream, line)) {
    base::TrimWhitespaceASCII(line, base::TRIM_ALL, &line);
    if (!re2::RE2::FullMatch(line, comment_re, &key, &value)) {
      break;
    }

    key = base::ToLowerASCII(key);
    if (key == "homepage") {
      homepage = value;
    } else if (key == "redirect") {
      auto url = GURL(value);
      if (url.is_valid()) {
        redirect_url = url;
      } else {
        VLOG(1) << "[eyeo] Invalid redirect URL: " << value
                << ". Will not redirect.";
      }
    } else if (key == "title") {
      title = value;
    } else if (key == "version") {
      version = value;
    } else if (key == "expires") {
      expires = ParseExpirationTime(value);
    }

    position_in_stream = filter_stream.tellg();
  }

  // NOTE: Rewind stream after last header line
  filter_stream.seekg(position_in_stream, std::ios_base::beg);

  return Metadata(std::move(homepage), std::move(title), std::move(version),
                  std::move(redirect_url), std::move(expires));
}

// static
Metadata Metadata::Default() {
  Metadata metadata;
  return metadata;
}

Metadata::Metadata(std::string homepage,
                   std::string title,
                   std::string version,
                   absl::optional<GURL> redirect_url,
                   base::TimeDelta expires)
    : homepage(std::move(homepage)),
      title(std::move(title)),
      version(std::move(version)),
      redirect_url(std::move(redirect_url)),
      expires(std::move(expires)) {}

Metadata::Metadata() : expires(kDefaultExpirationInterval) {}
Metadata::Metadata(const Metadata& other) = default;
Metadata::Metadata(Metadata&& other) = default;
Metadata::~Metadata() = default;

// static
bool Metadata::IsValidAdblockHeader(const std::string& adblock_header) {
  static re2::RE2 adblock_header_re("^\\[Adblock.*\\]");
  std::string adblock_header_trimmed;

  base::TrimWhitespaceASCII(adblock_header, base::TRIM_ALL,
                            &adblock_header_trimmed);
  if (!re2::RE2::FullMatch(re2::StringPiece(adblock_header_trimmed.data(),
                                            adblock_header_trimmed.size()),
                           adblock_header_re)) {
    return false;
  }
  return true;
}

// NOTE: This is done by the logic described here:
// https://eyeo.gitlab.io/adblockplus/abc/core-spec/#appendix-filter-list-syntax
// static
base::TimeDelta Metadata::ParseExpirationTime(
    const std::string& expiration_value) {
  static re2::RE2 expiration_time_re("\\s*([0-9]+)\\s*(h)?.*");
  std::string expiration_unit;
  uint64_t expiration_time;

  if (!re2::RE2::FullMatch(expiration_value, expiration_time_re,
                           &expiration_time, &expiration_unit)) {
    VLOG(1) << "[eyeo] Invalid expiration time format: " << expiration_value
            << ". Will use default value of "
            << kDefaultExpirationInterval.InDays() << " days.";
    return kDefaultExpirationInterval;
  }
  return base::ranges::clamp(expiration_unit == "h"
                                 ? base::Hours(expiration_time)
                                 : base::Days(expiration_time),
                             kMinExpirationInterval, kMaxExpirationInterval);
}

}  // namespace adblock
