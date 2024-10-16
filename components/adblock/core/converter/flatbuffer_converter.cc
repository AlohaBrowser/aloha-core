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

#include "components/adblock/core/converter/flatbuffer_converter.h"

#include <fstream>
#include <iostream>
#include <string_view>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/converter/parser/content_filter.h"
#include "components/adblock/core/converter/parser/filter_classifier.h"
#include "components/adblock/core/converter/parser/metadata.h"
#include "components/adblock/core/converter/parser/snippet_filter.h"
#include "components/adblock/core/converter/parser/url_filter.h"
#include "components/adblock/core/converter/serializer/flatbuffer_serializer.h"

namespace adblock {

namespace {
size_t GetActualSeparatorLength(const std::string_view filter_str,
                                FilterType filter_type) {
  if (filter_type == FilterType::Remove ||
      filter_type == FilterType::InlineCss) {
    if (base::StartsWith(filter_str, kElemHideFilterSeparator)) {
      return 2;
    }
    return 3;
  }
  if (filter_type == FilterType::ElemHide) {
    return 2;
  }
  return 3;
}
}  // namespace

static constexpr char kCommentPrefix[] = "!";
static constexpr size_t kMaxSeparatorLength = 3u;

// static
ConversionResult FlatbufferConverter::Convert(std::istream& filter_stream,
                                              GURL subscription_url,
                                              bool allow_privileged) {
  if (!filter_stream) {
    return ConversionError("Invalid filter stream");
  }

  auto metadata = Metadata::FromStream(filter_stream);
  if (!metadata.has_value()) {
    return ConversionError("Invalid filter list metadata");
  }

  if (metadata->redirect_url.has_value()) {
    return metadata->redirect_url.value();
  }

  FlatbufferSerializer flatbuffer_serializer(subscription_url,
                                             allow_privileged);
  flatbuffer_serializer.SerializeMetadata(std::move(metadata.value()));
  std::string line;
  while (std::getline(filter_stream, line)) {
    ConvertFilter(line, flatbuffer_serializer);
  }

  return flatbuffer_serializer.GetSerializedSubscription();
}

// static
std::unique_ptr<FlatbufferData> FlatbufferConverter::Convert(
    const std::vector<std::string>& filters,
    GURL subscription_url,
    bool allow_privileged) {
  FlatbufferSerializer flatbuffer_serializer(subscription_url,
                                             allow_privileged);
  for (const auto& filter : filters) {
    ConvertFilter(filter, flatbuffer_serializer);
  }

  return flatbuffer_serializer.GetSerializedSubscription();
}

// static
void FlatbufferConverter::ConvertFilter(
    const std::string& line,
    FlatbufferSerializer& flatbuffer_serializer) {
  const std::string_view filter_str =
      base::TrimWhitespaceASCII(line, base::TRIM_ALL);
  if (base::StartsWith(filter_str, kCommentPrefix) || filter_str.empty()) {
    return;
  }

  auto separator_pos = filter_str.find('#');
  FilterType filter_type = FilterType::Url;
  if (separator_pos != std::string::npos) {
    filter_type = FilterClassifier::Classify(filter_str.substr(separator_pos));
  }

  switch (filter_type) {
    case FilterType::ElemHide:
    case FilterType::ElemHideException:
    case FilterType::ElemHideEmulation:
    case FilterType::Remove:
    case FilterType::InlineCss:
      if (auto content_filter = ContentFilter::FromString(
              filter_str.substr(0, separator_pos), filter_type,
              filter_str.substr(
                  separator_pos +
                  (GetActualSeparatorLength(filter_str.substr(separator_pos),
                                            filter_type))))) {
        flatbuffer_serializer.SerializeContentFilter(
            std::move(content_filter.value()));
      } else {
        VLOG(1) << "[eyeo] Invalid content filter: " << line;
      }
      break;
    case FilterType::Snippet:
      if (auto snippet_filter = SnippetFilter::FromString(
              filter_str.substr(0, separator_pos),
              filter_str.substr(separator_pos + kMaxSeparatorLength))) {
        flatbuffer_serializer.SerializeSnippetFilter(
            std::move(snippet_filter.value()));
      } else {
        VLOG(1) << "[eyeo] Invalid snippet filter: " << line;
      }
      break;
    case FilterType::Url:
      if (auto url_filter = UrlFilter::FromString(
              std::string(filter_str.data(), filter_str.size()))) {
        flatbuffer_serializer.SerializeUrlFilter(std::move(url_filter.value()));
      } else {
        VLOG(1) << "[eyeo] Invalid url filter: " << line;
      }
      break;
  }
}

}  // namespace adblock
