// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "aw_bromium_client_bridge.h"

#include "android_webview/browser_jni_headers/BromiumClientBridge_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertUTF8ToJavaString;
using base::android::ToJavaArrayOfStrings;

namespace aloha {

void AwBromiumClientBridge::OnMediaPlay(
    const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url,
    const std::string& document_url, double duration_s, bool is_audio_only) {
  JNIEnv* env = AttachCurrentThread();

  auto obj = get_java_ref(env);
  if (!obj)
    return;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_media_url = ConvertUTF8ToJavaString(env, media_url);
  ScopedJavaLocalRef<jstring> j_document_url = ConvertUTF8ToJavaString(env, document_url);

  Java_BromiumClientBridge_onMediaPlay(env, obj,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_media_url,
    j_document_url, duration_s, is_audio_only);
}

void AwBromiumClientBridge::OnMediaPause(
    const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url,
    const std::string& document_url, double duration_s, bool is_audio_only) {
  JNIEnv* env = AttachCurrentThread();

  auto obj = get_java_ref(env);
  if (!obj)
    return;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_media_url = ConvertUTF8ToJavaString(env, media_url);
  ScopedJavaLocalRef<jstring> j_document_url = ConvertUTF8ToJavaString(env, document_url);

  Java_BromiumClientBridge_onMediaPause(env, obj,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_media_url,
    j_document_url, duration_s, is_audio_only);
}

void AwBromiumClientBridge::OnMediaDestroy(
    const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did, const std::string& media_url, const std::string& document_url) {
  JNIEnv* env = AttachCurrentThread();

  auto obj = get_java_ref(env);
  if (!obj)
    return;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_media_url = ConvertUTF8ToJavaString(env, media_url);
  ScopedJavaLocalRef<jstring> j_document_url = ConvertUTF8ToJavaString(env, document_url);

  Java_BromiumClientBridge_onMediaDestroy(env, obj,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_media_url,
    j_document_url);
}

// ALOHA https://app.clickup.com/t/2rqdtxz
void AwBromiumClientBridge::OnMediaError(
    const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
    const std::string& pipeline_status, const std::string& media_url,
    const std::string& document_url, double current_time_s, double duration_s) {
  JNIEnv* env = AttachCurrentThread();

  auto obj = get_java_ref(env);
  if (!obj)
    return;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_pipeline_status = ConvertUTF8ToJavaString(env, pipeline_status);
  ScopedJavaLocalRef<jstring> j_media_url = ConvertUTF8ToJavaString(env, media_url);
  ScopedJavaLocalRef<jstring> j_document_url = ConvertUTF8ToJavaString(env, document_url);

  Java_BromiumClientBridge_onMediaError(env, obj,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_pipeline_status,
    j_media_url, j_document_url, current_time_s, duration_s);
}

bool AwBromiumClientBridge::ShouldHideControlsInFullscreen(
    const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
    const std::string& url, double duration_s, const std::string& pending_elem_class) {
  JNIEnv* env = AttachCurrentThread();
  auto obj = get_java_ref(env);
  if (!obj)
    return false;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url);
  ScopedJavaLocalRef<jstring> j_pending_elem_class = ConvertUTF8ToJavaString(env, pending_elem_class);

  return Java_BromiumClientBridge_shouldHideControlsInFullscreen(env, obj,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_url, duration_s,
    j_pending_elem_class);
}

void AwBromiumClientBridge::OnVideoFullscreenChanged(
    bool is_fullscreen, const media::mojom::MediaPlayerId& player_id, int cid, int rid, int did,
    const std::string& url, bool media_controls_is_hidden) {
  JNIEnv* env = AttachCurrentThread();
  auto obj = get_java_ref(env);
  if (!obj)
    return;

  ScopedJavaLocalRef<jstring> j_html_id = ConvertUTF8ToJavaString(env, player_id.html_id);
  auto j_iframe_path = ToJavaArrayOfStrings(
    env, player_id.iframes_ids);
  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url);
  
  Java_BromiumClientBridge_onVideoFullscreenChanged(env, obj, is_fullscreen,
    j_html_id, j_iframe_path,
    cid, rid, did,
    j_url, media_controls_is_hidden);
}

void AwBromiumClientBridge::OnInternalDownloadStarted(const std::string& http_method, int size_in_bytes) {
  JNIEnv* env = AttachCurrentThread();
  auto obj = get_java_ref(env);
  if (!obj)
    return;
  
  ScopedJavaLocalRef<jstring> j_http_method = ConvertUTF8ToJavaString(env, http_method);
  Java_BromiumClientBridge_onInternalDownloadStarted(env, obj, j_http_method, size_in_bytes);  
}

bool AwBromiumClientBridge::ShouldPlayBackgroundVideo(){
  JNIEnv* env = AttachCurrentThread();
  auto obj = get_java_ref(env);
  if (!obj)
    return false;
  
  return Java_BromiumClientBridge_shouldPlayBackgroundVideo(env, obj);
}

} // namespace aloha
