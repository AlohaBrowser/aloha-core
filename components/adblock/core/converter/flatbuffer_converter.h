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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_FLATBUFFER_CONVERTER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_FLATBUFFER_CONVERTER_H_

#include <istream>
#include <memory>

#include "base/types/strong_alias.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "url/gurl.h"

namespace adblock {

using ConversionError =
    base::StrongAlias<class ConversionErrorTag, std::string>;
// Conversion can yield valid FlatbufferData, a redirect URL or an error:
using ConversionResult =
    absl::variant<std::unique_ptr<FlatbufferData>, GURL, ConversionError>;

class FlatbufferSerializer;
class FlatbufferConverter {
 public:
  static ConversionResult Convert(std::istream& filter_stream,
                                  GURL subscription_url,
                                  bool allow_privileged);
  static std::unique_ptr<FlatbufferData> Convert(
      const std::vector<std::string>& filters,
      GURL subscription_url,
      bool allow_privileged);

 private:
  static void ConvertFilter(const std::string& line,
                            FlatbufferSerializer& flatbuffer_serializer);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_FLATBUFFER_CONVERTER_H_
