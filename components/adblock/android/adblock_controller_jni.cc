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

#include <algorithm>
#include <iterator>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/logging.h"
#include "components/adblock/android/jni_headers/AdblockController_jni.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/browser_thread.h"

using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::JavaParamRef;
using base::android::MethodID;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfObjects;
using base::android::ToJavaArrayOfStrings;

namespace {

ScopedJavaLocalRef<jobject> ToJava(JNIEnv* env,
                                   ScopedJavaLocalRef<jclass>& url_class,
                                   jmethodID& url_constructor,
                                   const std::string& url,
                                   const std::string& title,
                                   const std::string& version,
                                   const std::vector<std::string>& languages,
                                   const bool autoinstalled) {
  ScopedJavaLocalRef<jobject> url_param(
      env, env->NewObject(url_class.obj(), url_constructor,
                          ConvertUTF8ToJavaString(env, url).obj()));
  CheckException(env);
  return Java_Subscription_Constructor(env, url_param,
                                       ConvertUTF8ToJavaString(env, title),
                                       ConvertUTF8ToJavaString(env, version),
                                       ToJavaArrayOfStrings(env, languages),
                                       autoinstalled ? JNI_TRUE : JNI_FALSE);
}

std::vector<ScopedJavaLocalRef<jobject>> CSubscriptionsToJObjects(
    JNIEnv* env,
    const std::vector<scoped_refptr<adblock::Subscription>>& subscriptions) {
  ScopedJavaLocalRef<jclass> url_class = GetClass(env, "java/net/URL");
  jmethodID url_constructor = MethodID::Get<MethodID::TYPE_INSTANCE>(
      env, url_class.obj(), "<init>", "(Ljava/lang/String;)V");
  std::vector<ScopedJavaLocalRef<jobject>> jobjects;
  jobjects.reserve(subscriptions.size());
  for (auto& sub : subscriptions) {
    jobjects.push_back(ToJava(
        env, url_class, url_constructor, sub->GetSourceUrl().spec(),
        sub->GetTitle(), sub->GetCurrentVersion(), std::vector<std::string>{},
        sub->GetInstallationState() ==
            adblock::InstalledSubscription::InstallationState::AutoInstalled));
  }
  return jobjects;
}

std::vector<ScopedJavaLocalRef<jobject>> CSubscriptionsToJObjects(
    JNIEnv* env,
    std::vector<adblock::KnownSubscriptionInfo>& subscriptions) {
  ScopedJavaLocalRef<jclass> url_class = GetClass(env, "java/net/URL");
  jmethodID url_constructor = MethodID::Get<MethodID::TYPE_INSTANCE>(
      env, url_class.obj(), "<init>", "(Ljava/lang/String;)V");
  std::vector<ScopedJavaLocalRef<jobject>> jobjects;
  jobjects.reserve(subscriptions.size());
  for (auto& sub : subscriptions) {
    if (sub.ui_visibility == adblock::SubscriptionUiVisibility::Visible) {
      // The checks here are when one makes f.e. adblock:custom visible
      DCHECK(sub.url.is_valid());
      if (sub.url.is_valid()) {
        jobjects.push_back(ToJava(env, url_class, url_constructor,
                                  sub.url.spec(), sub.title, "", sub.languages,
                                  false));
      }
    }
  }
  return jobjects;
}

}  // namespace

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetInstalledSubscriptions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          content::BrowserContextFromJavaHandle(jbrowser_context_handle));
  if (!subscription_service) {
    return ToJavaArrayOfObjects(env,
                                std::vector<ScopedJavaLocalRef<jobject>>{});
  }

  return ToJavaArrayOfObjects(
      env, CSubscriptionsToJObjects(
               env, subscription_service->GetCurrentSubscriptions(
                        subscription_service->GetFilteringConfiguration(
                            adblock::kAdblockFilteringConfigurationName))));
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetRecommendedSubscriptions(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto list = adblock::config::GetKnownSubscriptions();
  return ToJavaArrayOfObjects(env, CSubscriptionsToJObjects(env, list));
}
