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

#include "components/adblock/android/resource_classification_notifier_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "components/adblock/android/jni_headers/ResourceClassificationNotifier_jni.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;

namespace adblock {

ResourceClassificationNotifierAndroid::ResourceClassificationNotifierAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcontroller,
    ResourceClassificationRunner* classification_runner)
    : classification_runner_(classification_runner),
      java_weak_controller_(env, jcontroller) {
  if (classification_runner_) {
    classification_runner_->AddObserver(this);
  }
}

ResourceClassificationNotifierAndroid::
    ~ResourceClassificationNotifierAndroid() {
  if (classification_runner_) {
    classification_runner_->RemoveObserver(this);
  }
}

void ResourceClassificationNotifierAndroid::OnRequestMatched(
    const GURL& url,
    FilterMatchResult result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  DCHECK(result == FilterMatchResult::kBlockRule ||
         result == FilterMatchResult::kAllowRule);
  const bool was_blocked = result == FilterMatchResult::kBlockRule;
  DVLOG(3) << "[eyeo] Ad matched " << url << "(type: " << content_type
           << (was_blocked ? ", blocked" : ", allowed") << ")";
  JNIEnv* env = AttachCurrentThread();
  auto java_controller = java_weak_controller_.get(env);
  if (!java_controller.is_null()) {
    ScopedJavaLocalRef<jstring> j_url =
        ConvertUTF8ToJavaString(env, url.spec());
    ScopedJavaLocalRef<jobjectArray> j_parents = ToJavaArrayOfStrings(
        env, adblock::utils::ConvertURLs(parent_frame_urls));
    ScopedJavaLocalRef<jstring> j_subscription =
        ConvertUTF8ToJavaString(env, subscription.spec());
    ScopedJavaLocalRef<jstring> j_configuration =
        ConvertUTF8ToJavaString(env, configuration_name);
    const content::GlobalRenderFrameHostId rfh_id =
        render_frame_host->GetGlobalId();
    Java_ResourceClassificationNotifier_requestMatchedCallback(
        env, java_controller, j_url, was_blocked, j_parents, j_subscription,
        j_configuration, static_cast<int>(content_type), rfh_id.child_id,
        rfh_id.frame_routing_id);
  }
}

void ResourceClassificationNotifierAndroid::OnPageAllowed(
    const GURL& url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  DVLOG(3) << "[eyeo] Page allowed " << url;
  JNIEnv* env = AttachCurrentThread();
  auto java_controller = java_weak_controller_.get(env);
  if (!java_controller.is_null()) {
    ScopedJavaLocalRef<jstring> j_url =
        ConvertUTF8ToJavaString(env, url.spec());
    ScopedJavaLocalRef<jstring> j_subscription =
        ConvertUTF8ToJavaString(env, subscription.spec());
    ScopedJavaLocalRef<jstring> j_configuration =
        ConvertUTF8ToJavaString(env, configuration_name);
    const content::GlobalRenderFrameHostId rfh_id =
        render_frame_host->GetGlobalId();
    Java_ResourceClassificationNotifier_pageAllowedCallback(
        env, java_controller, j_url, j_subscription, j_configuration,
        rfh_id.child_id, rfh_id.frame_routing_id);
  }
}

void ResourceClassificationNotifierAndroid::OnPopupMatched(
    const GURL& url,
    FilterMatchResult result,
    const GURL& opener_url,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription,
    const std::string& configuration_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  DCHECK(result == FilterMatchResult::kBlockRule ||
         result == FilterMatchResult::kAllowRule);
  const bool was_blocked = result == FilterMatchResult::kBlockRule;
  DVLOG(3) << "[eyeo] Popup matched " << url
           << (was_blocked ? ", blocked" : ", allowed");
  JNIEnv* env = AttachCurrentThread();
  auto java_controller = java_weak_controller_.get(env);
  if (!java_controller.is_null()) {
    ScopedJavaLocalRef<jstring> j_url =
        ConvertUTF8ToJavaString(env, url.spec());
    ScopedJavaLocalRef<jstring> j_opener =
        ConvertUTF8ToJavaString(env, opener_url.spec());
    ScopedJavaLocalRef<jstring> j_subscription =
        ConvertUTF8ToJavaString(env, subscription.spec());
    ScopedJavaLocalRef<jstring> j_configuration =
        ConvertUTF8ToJavaString(env, configuration_name);
    const content::GlobalRenderFrameHostId rfh_id =
        render_frame_host->GetGlobalId();
    Java_ResourceClassificationNotifier_popupMatchedCallback(
        env, java_controller, j_url, was_blocked, j_opener, j_subscription,
        j_configuration, rfh_id.child_id, rfh_id.frame_routing_id);
  }
}

}  // namespace adblock

static jlong JNI_ResourceClassificationNotifier_Create(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcontroller,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  DCHECK(!jcontroller.is_null());
  DCHECK(!jbrowser_context_handle.is_null());
  return reinterpret_cast<jlong>(
      new adblock::ResourceClassificationNotifierAndroid(
          env, std::move(jcontroller),
          adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
              content::BrowserContextFromJavaHandle(jbrowser_context_handle))));
}
