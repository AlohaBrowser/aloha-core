// Copyright 2026 Aloha Mobile Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
//
// ALOHA https://app.clickup.com/t/z93epaa335 Per-tab cache of HLS manifests.

#ifndef ALOHA_SRC_NATIVE_HLS_MANIFEST_CACHE_H_
#define ALOHA_SRC_NATIVE_HLS_MANIFEST_CACHE_H_

#include <map>
#include <string>

#include "base/containers/lru_cache.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace aloha {

// Bodies of HLS manifests captured by the network service while the page's
// player fetched them (URLLoader::CompletePendingWrite →
// URLLoaderNetworkServiceObserver::OnHlsManifestLoaded →
// AwContentBrowserClient::OnHlsManifestLoaded). Gives the Kotlin downloader
// the text the player already received, so it does not have to re-request
// it: PH signs playlist URLs per client IP + Referer and is testing
// single-use ones (CU-z93epaa335), and a re-request is a wasted round trip
// anyway.
//
// Lives exactly as long as its WebContents. Bounded by entry count and total
// bytes so that live streams, whose media playlists are re-fetched every few
// seconds under fresh URLs, cannot grow it without limit; the oldest entries
// are evicted first.
class HlsManifestCache
    : public content::WebContentsUserData<HlsManifestCache> {
 public:
  // Typical playlists are 3-8 KiB; a 4-hour VOD with signed URLs ~2 MiB.
  // Must fit at least one manifest of URLLoader's kMaxHlsManifestBytes (4 MiB).
  static constexpr size_t kMaxEntries = 32;
  static constexpr size_t kMaxTotalBytes = 8 * 1024 * 1024;

  HlsManifestCache(const HlsManifestCache&) = delete;
  HlsManifestCache& operator=(const HlsManifestCache&) = delete;
  ~HlsManifestCache() override;

  // Stores (or replaces) the manifest for |url|. Key = URL without fragment,
  // without the LL-HLS delivery directives (_HLS_msn/_HLS_part/_HLS_skip) and
  // without an empty query; the rest of the query (signatures) must match.
  void Put(const GURL& url, std::string content);

  // Makes |alias| (e.g. the pre-redirect URL) resolve to the manifest stored
  // under |target|. No bytes are counted for an alias. A manifest previously
  // stored under |alias| itself is dropped: the URL now redirects, so the
  // newer content wins.
  void AddAlias(const GURL& alias, const GURL& target);

  // Manifest text for |url| or nullptr. Marks the entry as recently used.
  // The pointer is valid until the next Put()/AddAlias() on this cache.
  const std::string* Get(const GURL& url);

  size_t size() const { return entries_.size(); }
  size_t total_bytes() const { return total_bytes_; }

 private:
  friend class content::WebContentsUserData<HlsManifestCache>;
  explicit HlsManifestCache(content::WebContents* web_contents);

  static std::string Key(const GURL& url);
  void EvictIfNeeded();

  void EraseEntry(base::LRUCache<std::string, std::string>::iterator it);

  base::LRUCache<std::string, std::string> entries_;
  // alias key → canonical key present in |entries_|.
  std::map<std::string, std::string> aliases_;
  size_t total_bytes_ = 0;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace aloha

#endif  // ALOHA_SRC_NATIVE_HLS_MANIFEST_CACHE_H_
