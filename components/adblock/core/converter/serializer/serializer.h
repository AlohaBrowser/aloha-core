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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_SERIALIZER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_SERIALIZER_H_

#include <memory>

namespace adblock {

class ContentFilter;
class Metadata;
class SnippetFilter;
class UrlFilter;

class Serializer {
 public:
  virtual ~Serializer() = default;
  virtual void SerializeMetadata(const Metadata metadata) = 0;
  virtual void SerializeContentFilter(const ContentFilter content_filter) = 0;
  virtual void SerializeSnippetFilter(const SnippetFilter snippet_filter) = 0;
  virtual void SerializeUrlFilter(const UrlFilter url_filter) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_SERIALIZER_H_
