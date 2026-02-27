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
// ALOHA CU-86eqe7rd2 (U3): native part of the profile API for Kotlin.

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_browser_context_store.h"
#include "android_webview/browser/aw_render_process.h"
#include "base/android/jni_string.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/render_process_host.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/browser_jni_headers/BromiumProfiles_jni.h"

namespace aloha {

// Full data wipe for a named profile. "Live": the profile may be instantiated
// and have open tabs (they keep running; in-memory document state is not
// touched). If the profile does not exist — no-op.
static void JNI_BromiumProfiles_ClearProfileData(JNIEnv* env,
                                                 std::string name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  android_webview::AwBrowserContext* context =
      android_webview::AwBrowserContextStore::GetInstance()->Get(
          name, /*create_if_needed=*/false);
  if (!context) {
    return;
  }

  // Content layer: cookies (incl. session), DOM storage (localStorage,
  // sessionStorage, IndexedDB, CacheStorage, FileSystem, Service Workers,
  // background fetch), HTTP cache, downloads. Both origin categories
  // (unprotected + protected web).
  const uint64_t remove_mask =
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE |
      content::BrowsingDataRemover::DATA_TYPE_COOKIES |
      content::BrowsingDataRemover::DATA_TYPE_CACHE |
      content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS;
  context->GetBrowsingDataRemover()->Remove(
      base::Time(), base::Time::Max(), remove_mask,
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
          content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB);

  // Permissions/site settings (geolocation) live in per-profile
  // SharedPreferences and are not covered by BrowsingDataRemover — cleared from
  // Java (BromiumProfiles.clearProfileData → AwGeolocationPermissions.clearAll).
}

// Full cache wipe for a named profile. "Live": the profile may be instantiated
static void JNI_BromiumProfiles_ClearProfileCache(JNIEnv* env,
                                                  std::string name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  android_webview::AwBrowserContext* context =
      android_webview::AwBrowserContextStore::GetInstance()->Get(
          name, /*create_if_needed=*/false);
  if (!context) {
    return;
  }

  // Renderer-cleaner
  for (auto it = content::RenderProcessHost::AllHostsIterator(); !it.IsAtEnd(); it.Advance()) {
    auto* rph = it.GetCurrentValue();
    if (rph->GetBrowserContext() != context || !rph->IsInitializedAndNotDead())
      continue;
    android_webview::AwRenderProcess::GetInstanceForRenderProcessHost(rph)->ClearCache();
  }


  context->GetBrowsingDataRemover()->Remove(
      base::Time(), base::Time::Max(),
      content::BrowsingDataRemover::DATA_TYPE_CACHE,
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
          content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB);
}


// Full Service Worker data wipe for a named profile. "Live": the profile may be instantiated
static void JNI_BromiumProfiles_ClearServiceWorkerData(JNIEnv* env,
                                                        std::string name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  android_webview::AwBrowserContext* context =
      android_webview::AwBrowserContextStore::GetInstance()->Get(
          name, /*create_if_needed=*/false);
    if (!context) {
        return;
    }

    context->GetBrowsingDataRemover()->Remove(
        base::Time(), base::Time::Max(),
        content::BrowsingDataRemover::DATA_TYPE_SERVICE_WORKERS,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
            content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB);

}

}  // namespace aloha

DEFINE_JNI(BromiumProfiles)
