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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_CONTENT_TYPE_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_CONTENT_TYPE_H_

#include <string>

namespace adblock {

enum ContentType {
  Unknown = 0,
  Other = 1,
  Script = 2,
  Image = 4,
  Stylesheet = 8,
  Object = 16,
  Subdocument = 32,
  Websocket = 128,
  Webrtc = 256,
  Ping = 1024,
  Xmlhttprequest = 2048,
  Media = 16384,
  Font = 32768,
  WebBundle = 65536,
  Default = (1 << 24) - 1,
};

std::string ContentTypeToString(ContentType content_type);
ContentType ContentTypeFromString(const std::string& content_type);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_CONTENT_TYPE_H_
