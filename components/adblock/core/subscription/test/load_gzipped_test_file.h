
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_LOAD_GZIPPED_TEST_FILE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_LOAD_GZIPPED_TEST_FILE_H_

#include <string>
#include <string_view>

namespace adblock {

// Loads and extracts a file from components/test/data/adblock/|filename|
// The file is assumed to exist and be gzipped. The function CHECKs and will
// crash otherwise.
std::string LoadGzippedTestFile(std::string_view filename);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_LOAD_GZIPPED_TEST_FILE_H_
