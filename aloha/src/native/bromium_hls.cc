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
// ALOHA https://app.clickup.com/t/z93epaa335 Native part of BromiumHls.java.

#include <string>

#include "aloha/src/native/hls_manifest_cache.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/browser_jni_headers/BromiumHls_jni.h"

namespace aloha {

// Manifest text cached for |url| in the tab behind |j_web_contents|, or null.
static base::android::ScopedJavaLocalRef<jstring>
JNI_BromiumHls_GetCachedManifest(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& j_web_contents,
    const std::string& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(j_web_contents);
  if (!web_contents) {
    return nullptr;
  }
  auto* cache = HlsManifestCache::FromWebContents(web_contents);
  if (!cache) {
    return nullptr;
  }
  const std::string* content = cache->Get(GURL(url));
  if (!content) {
    return nullptr;
  }
  return base::android::ConvertUTF8ToJavaString(env, *content);
}

}  // namespace aloha

DEFINE_JNI(BromiumHls)
