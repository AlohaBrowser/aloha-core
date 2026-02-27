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

#include "components/eyeo_testing/test_request_reader.h"

#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace adblock {
namespace test {

absl::optional<int64_t> ParseContentType(std::string_view input) {
  if (input == "OTHER") {
    return 1;
  }
  if (input == "SCRIPT") {
    return 2;
  }
  if (input == "IMAGE") {
    return 4;
  }
  if (input == "STYLESHEET") {
    return 8;
  }
  if (input == "OBJECT") {
    return 16;
  }
  if (input == "SUBDOCUMENT") {
    return 32;
  }
  if (input == "WEBSOCKET") {
    return 128;
  }
  if (input == "WEBRTC") {
    return 256;
  }
  if (input == "PING") {
    return 1024;
  }
  if (input == "XMLHTTPREQUEST") {
    return 2048;
  }
  if (input == "MEDIA") {
    return 16384;
  }
  if (input == "FONT") {
    return 32768;
  }
  if (input == "POPUP") {
    return 1 << 24;
  }
  if (input == "CSP") {
    return 1 << 25;  // unused
  }
  if (input == "HEADER") {
    return 1 << 26;  // unused
  }
  if (input == "DOCUMENT") {
    return 1 << 27;
  }
  if (input == "GENERICBLOCK") {
    return 1 << 28;
  }
  if (input == "ELEMHIDE") {
    return 1 << 29;
  }
  if (input == "GENERICHIDE") {
    return 1 << 30;
  }
  return absl::nullopt;
}
TestRequestReader::TestRequestReader() = default;
TestRequestReader::~TestRequestReader() = default;

bool TestRequestReader::Open(base::FilePath path) {
  current_line_number_ = 1;
  input_stream_.open(path.AsUTF8Unsafe());
  return input_stream_.is_open();
}

void TestRequestReader::FastForwardLines(int number_of_lines_to_skip) {
  std::string line;
  while (number_of_lines_to_skip > 0) {
    std::getline(input_stream_, line);
    number_of_lines_to_skip--;
    current_line_number_++;
  }
}

absl::optional<Request> TestRequestReader::GetNextRequest() {
  std::string line;
  if (!std::getline(input_stream_, line)) {
    return absl::nullopt;
  }
  auto split_line = base::SplitStringPiece(line, " \t", base::TRIM_WHITESPACE,
                                           base::SPLIT_WANT_NONEMPTY);
  if (split_line.size() != 3) {
    return absl::nullopt;
  }
  auto content_type = ParseContentType(split_line[1]);
  if (!content_type) {
    return absl::nullopt;
  }
  Request request;
  request.domain = split_line[0];
  request.content_type = *content_type;
  request.url = GURL(split_line[2]);
  request.line_number = current_line_number_++;
  return request;
}

}  // namespace test
}  // namespace adblock
