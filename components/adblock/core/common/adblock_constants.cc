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

#include "components/adblock/core/common/adblock_constants.h"

#include "base/base64.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/adblock/core/schema/schema_hash.h"

namespace adblock {

const char kSiteKeyHeaderKey[] = "x-adblock-key";

const char kAllowlistEverythingFilter[] = "@@*$document";

const char kAdblockFilteringConfigurationName[] = "adblock";

const char kBlankHtml[] =
    "data:text/html,<!DOCTYPE html><html><head></head><body></body></html>";

const char kBlankMp3[] =
    "data:audio/"
    "mpeg;base64,SUQzBAAAAAAAI1RTU0UAAAAPAAADTGF2ZjU4LjIwLjEwMAAAAAAAAAAAAAAA//"
    "tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAASW5mbwAAAA8AAAAGAAADAABgYGBg"
    "YGBgYGBgYGBgYGBggICAgICAgICAgICAgICAgICgoKCgoKCgoKCgoKCgoKCgwMDAwMDAwMDAwM"
    "DAwMDAwMDg4ODg4ODg4ODg4ODg4ODg4P////////////////////"
    "8AAAAATGF2YzU4LjM1AAAAAAAAAAAAAAAAJAYAAAAAAAAAAwDVxttG//"
    "sUZAAP8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAETEFNRTMuMTAwVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//"
    "sUZB4P8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAEVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//"
    "sUZDwP8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAEVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//"
    "sUZFoP8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAEVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//"
    "sUZHgP8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAEVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//"
    "sUZJYP8AAAaQAAAAgAAA0gAAABAAABpAAAACAAADSAAAAEVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV";

const char kBlankMp4[] =
    "data:video/"
    "mp4;base64,"
    "AAAAIGZ0eXBpc29tAAACAGlzb21pc28yYXZjMW1wNDEAAAAIZnJlZQAAAuJtZGF0AAACrwYF//"
    "+r3EXpvebZSLeWLNgg2SPu73gyNjQgLSBjb3JlIDE2MCByMzAxMU0gY2RlOWE5MyAtIEguMjY0"
    "L01QRUctNCBBVkMgY29kZWMgLSBDb3B5bGVmdCAyMDAzLTIwMjAgLSBodHRwOi8vd3d3LnZpZG"
    "VvbGFuLm9yZy94MjY0Lmh0bWwgLSBvcHRpb25zOiBjYWJhYz0xIHJlZj0zIGRlYmxvY2s9MTow"
    "OjAgYW5hbHlzZT0weDM6MHgxMTMgbWU9aGV4IHN1Ym1lPTcgcHN5PTEgcHN5X3JkPTEuMDA6MC"
    "4wMCBtaXhlZF9yZWY9MSBtZV9yYW5nZT0xNiBjaHJvbWFfbWU9MCB0cmVsbGlzPTEgOHg4ZGN0"
    "PTEgY3FtPTAgZGVhZHpvbmU9MjEsMTEgZmFzdF9wc2tpcD0xIGNocm9tYV9xcF9vZmZzZXQ9LT"
    "IgdGhyZWFkcz0yIGxvb2thaGVhZF90aHJlYWRzPTEgc2xpY2VkX3RocmVhZHM9MCBucj0wIGRl"
    "Y2ltYXRlPTEgaW50ZXJsYWNlZD0wIGJsdXJheV9jb21wYXQ9MCBjb25zdHJhaW5lZF9pbnRyYT"
    "0wIGJmcmFtZXM9MyBiX3B5cmFtaWQ9MiBiX2FkYXB0PTEgYl9iaWFzPTAgZGlyZWN0PTEgd2Vp"
    "Z2h0Yj0xIG9wZW5fZ29wPTAgd2VpZ2h0cD0yIGtleWludD0yNTAga2V5aW50X21pbj0yNSBzY2"
    "VuZWN1dD00MCBpbnRyYV9yZWZyZXNoPTAgcmNfbG9va2FoZWFkPTQwIHJjPWNyZiBtYnRyZWU9"
    "MSBjcmY9MjMuMCBxY29tcD0wLjYwIHFwbWluPTAgcXBtYXg9NjkgcXBzdGVwPTQgaXBfcmF0aW"
    "89MS40MCBhcT0xOjEuMDAAgAAAACNliIQAK//"
    "+9dvzLK5umjbe9jc2CT9EPcfnoOYC2tjtP+"
    "go4QAAAwRtb292AAAAbG12aGQAAAAAAAAAAAAAAAAAAAPoAAAAKAABAAABAAAAAAAAAAAAAAAA"
    "AQAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAACAAACLnRyYWsAAABcdGtoZAAAAAMAAAAAAAAAAAAAAAEAAAAAAAAAKAAAAAAAAAAA"
    "AAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAEAAAAAAZAAAADgAAAAAAC"
    "RlZHRzAAAAHGVsc3QAAAAAAAAAAQAAACgAAAAAAAEAAAAAAaZtZGlhAAAAIG1kaGQAAAAAAAAA"
    "AAAAAAAAADIAAAACAFXEAAAAAAAtaGRscgAAAAAAAAAAdmlkZQAAAAAAAAAAAAAAAFZpZGVvSG"
    "FuZGxlcgAAAAFRbWluZgAAABR2bWhkAAAAAQAAAAAAAAAAAAAAJGRpbmYAAAAcZHJlZgAAAAAA"
    "AAABAAAADHVybCAAAAABAAABEXN0YmwAAACtc3RzZAAAAAAAAAABAAAAnWF2YzEAAAAAAAAAAQ"
    "AAAAAAAAAAAAAAAAAAAAAAZAA4AEgAAABIAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAY//8AAAA3YXZjQwFkAAr/"
    "4QAaZ2QACvNlHJ42JwEQAAADABAAAAMDIPEiWWABAAZo6+PLIsD8+"
    "PgAAAAAEHBhc3AAAAABAAAAAQAAABhzdHRzAAAAAAAAAAEAAAABAAACAAAAABxzdHNjAAAAAAA"
    "AAAEAAAABAAAAAQAAAAEAAAAUc3RzegAAAAAAAALaAAAAAQAAABRzdGNvAAAAAAAAAAEAAAAwA"
    "AAAYnVkdGEAAABabWV0YQAAAAAAAAAhaGRscgAAAAAAAAAAbWRpcmFwcGwAAAAAAAAAAAAAAAA"
    "taWxzdAAAACWpdG9vAAAAHWRhdGEAAAABAAAAAExhdmY1OC40NS4xMDA=";

const char kBlankGif[] =
    "data:image/gif;base64,R0lGODlhAQABAIABAAAAAP///"
    "yH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==";

const char kBlankPng2x2[] =
    "data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAC0lEQ"
    "VQI12NgQAcAABIAAe+JVKQAAAAASUVORK5CYII=";

const char kBlankPng3x2[] =
    "data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAYAAACddGYaAAAAC0lEQVQI12NgwAUAABoAASRETu"
    "UAAAAASUVORK5CYII=";

const char kBlankPng32x32[] =
    "data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAGklEQVRYw+"
    "3BAQEAAACCIP+vbkhAAQAAAO8GECAAAZf3V9cAAAAASUVORK5CYII=";

const std::string& CurrentSchemaVersion() {
  static std::string kCurrentSchemaVersion =
      base::Base64Encode(kSha256_filter_list_schema_generated_h);
  return kCurrentSchemaVersion;
}

const GURL& TestPagesSubscriptionUrl() {
  static GURL kTestPagesUrl(
      "https://abptestpages.org/en/abp-testcase-subscription.txt");
  return kTestPagesUrl;
}

const GURL& CustomFiltersUrl() {
  static GURL kCustomFiltersUrl("adblock:custom");
  return kCustomFiltersUrl;
}

std::string_view RewriteUrl(flat::AbpResource type) {
  switch (type) {
    case flat::AbpResource_BlankText:
      return "data:text/plain,";
    case flat::AbpResource_BlankCss:
      return "data:text/css,";
    case flat::AbpResource_BlankJs:
      return "data:application/javascript,";
    case flat::AbpResource_BlankHtml:
      return kBlankHtml;
    case flat::AbpResource_BlankMp3:
      return kBlankMp3;
    case flat::AbpResource_BlankMp4:
      return kBlankMp4;
    case flat::AbpResource_TransparentGif1x1:
      return kBlankGif;
    case flat::AbpResource_TransparentPng2x2:
      return kBlankPng2x2;
    case flat::AbpResource_TransparentPng3x2:
      return kBlankPng3x2;
    case flat::AbpResource_TransparentPng32x32:
      return kBlankPng32x32;
    default:
      return {};
  }
}

bool g_eyeo_disable_filtering_by_default = EYEO_DISABLE_FILTERING_BY_DEFAULT;

bool IsEyeoFilteringDisabledByDefault() {
  return g_eyeo_disable_filtering_by_default;
}

base::AutoReset<bool> OverrideEyeoFilteringDisabledByDefault(bool val) {
  return base::AutoReset<bool>(&g_eyeo_disable_filtering_by_default, val);
}

bool g_eyeo_disable_aa_by_default = EYEO_DISABLE_AA_BY_DEFAULT;

bool IsAcceptableAdsDisabledByDefault() {
  return g_eyeo_disable_aa_by_default;
}

base::AutoReset<bool> OverrideAcceptableAdsDisabledByDefault(bool val) {
  return base::AutoReset<bool>(&g_eyeo_disable_aa_by_default, val);
}

}  // namespace adblock
