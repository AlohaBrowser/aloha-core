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

#include "components/adblock/core/subscription/test/load_gzipped_test_file.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "third_party/zlib/google/compression_utils.h"

namespace adblock {

std::string LoadGzippedTestFile(std::string_view filename) {
  base::FilePath path;
  CHECK(base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &path));
  path = path.AppendASCII("components")
             .AppendASCII("test")
             .AppendASCII("data")
             .AppendASCII("adblock")
             .AppendASCII(filename);
  CHECK(base::PathExists(path));
  std::string content;
  CHECK(base::ReadFileToString(path, &content));
  CHECK(compression::GzipUncompress(content, &content));
  return content;
}

}  // namespace adblock
