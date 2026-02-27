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

#ifndef COMPONENTS_EYEO_TESTING_TEST_REQUEST_READER_H_
#define COMPONENTS_EYEO_TESTING_TEST_REQUEST_READER_H_

#include <fstream>
#include <string>

#include "base/files/file_path.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace adblock {
namespace test {

struct Request {
  int line_number;
  GURL url;
  int64_t content_type;
  std::string domain;
};

class TestRequestReader {
 public:
  TestRequestReader();
  ~TestRequestReader();
  bool Open(base::FilePath path);
  void FastForwardLines(int number_of_lines_to_skip);
  absl::optional<Request> GetNextRequest();

 private:
  int current_line_number_ = 0;
  std::fstream input_stream_;
};

}  // namespace test
}  // namespace adblock

#endif  // COMPONENTS_EYEO_TESTING_TEST_REQUEST_READER_H_
