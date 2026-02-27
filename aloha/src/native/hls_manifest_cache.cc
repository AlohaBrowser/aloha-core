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

#include "aloha/src/native/hls_manifest_cache.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#include <utility>

namespace aloha {

HlsManifestCache::HlsManifestCache(content::WebContents* web_contents)
    : content::WebContentsUserData<HlsManifestCache>(*web_contents),
      entries_(base::LRUCache<std::string, std::string>::NO_AUTO_EVICT) {}

HlsManifestCache::~HlsManifestCache() = default;

// static
std::string HlsManifestCache::Key(const GURL& url) {
  // Drop the fragment and the LL-HLS delivery directives (RFC 8216bis §6.2.5):
  // a low-latency player reloads the same media playlist ~2x/sec with fresh
  // _HLS_msn/_HLS_part values. Keyed verbatim, one live stream would flush
  // the whole cache within seconds; normalized, every reload just replaces
  // the previous playlist of that variant. An empty query ("x.m3u8?") is
  // dropped too so that lookups without the trailing '?' still hit.
  std::vector<std::string> kept;
  for (std::string_view pair : base::SplitStringPiece(
           url.query(), "&", base::TRIM_WHITESPACE,
           base::SPLIT_WANT_NONEMPTY)) {
    std::string_view name = pair.substr(0, pair.find('='));
    if (name == "_HLS_msn" || name == "_HLS_part" || name == "_HLS_skip") {
      continue;
    }
    kept.emplace_back(pair);
  }
  GURL::Replacements replacements;
  replacements.ClearRef();
  const std::string query = base::JoinString(kept, "&");
  if (query.empty()) {
    replacements.ClearQuery();
  } else {
    replacements.SetQueryStr(query);
  }
  return url.ReplaceComponents(replacements).spec();
}

void HlsManifestCache::Put(const GURL& url, std::string content) {
  if (!url.is_valid() || content.empty()) {
    return;
  }
  std::string key = Key(url);
  aliases_.erase(key);  // a real entry shadows an alias of the same name
  auto it = entries_.Peek(key);
  if (it != entries_.end()) {
    // Replace in place and keep the aliases pointing here: after a redirect
    // the player reloads the playlist by its final URL, while the app still
    // remembers the pre-redirect one.
    total_bytes_ -= it->second.size();
    entries_.Erase(it);
  }
  total_bytes_ += content.size();
  entries_.Put(std::move(key), std::move(content));
  EvictIfNeeded();
}

void HlsManifestCache::AddAlias(const GURL& alias, const GURL& target) {
  if (!alias.is_valid() || !target.is_valid()) {
    return;
  }
  std::string alias_key = Key(alias);
  std::string target_key = Key(target);
  if (alias_key == target_key || entries_.Peek(target_key) == entries_.end()) {
    return;
  }
  auto shadowed = entries_.Peek(alias_key);
  if (shadowed != entries_.end()) {
    EraseEntry(shadowed);
  }
  aliases_[std::move(alias_key)] = std::move(target_key);
}

const std::string* HlsManifestCache::Get(const GURL& url) {
  std::string key = Key(url);
  auto alias = aliases_.find(key);
  if (alias != aliases_.end()) {
    key = alias->second;
  }
  auto it = entries_.Get(key);
  return it == entries_.end() ? nullptr : &it->second;
}

void HlsManifestCache::EraseEntry(
    base::LRUCache<std::string, std::string>::iterator it) {
  total_bytes_ -= it->second.size();
  const std::string& key = it->first;
  for (auto a = aliases_.begin(); a != aliases_.end();) {
    a = (a->second == key) ? aliases_.erase(a) : std::next(a);
  }
  entries_.Erase(it);
}

void HlsManifestCache::EvictIfNeeded() {
  // LRUCache keeps the most recently used entry at begin(); evict from the
  // tail. Never evict the entry that was just inserted.
  while (entries_.size() > 1 &&
         (entries_.size() > kMaxEntries || total_bytes_ > kMaxTotalBytes)) {
    EraseEntry(std::prev(entries_.end()));
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(HlsManifestCache);

}  // namespace aloha
