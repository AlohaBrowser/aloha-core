#include <algorithm>
#include <iterator>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/logging.h"
#include "components/adblock/android/jni_headers/AdblockSubscriptionConfig_jni.h"
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

// ALOHA https://app.clickup.com/t/86eph6cjh
namespace {

ScopedJavaLocalRef<jobject> ToJava(JNIEnv* env,
                                   ScopedJavaLocalRef<jclass>& url_class,
                                   jmethodID& url_constructor,
                                   const std::string& url,
                                   const std::string& title,
                                   const std::string& version,
                                   const std::vector<std::string>& languages) {
  ScopedJavaLocalRef<jobject> url_param(
      env, env->NewObject(url_class.obj(), url_constructor,
                          ConvertUTF8ToJavaString(env, url).obj()));
  CheckException(env);
  return Java_Subscription_Constructor(env, url_param,
                                       ConvertUTF8ToJavaString(env, title),
                                       ConvertUTF8ToJavaString(env, version),
                                       ToJavaArrayOfStrings(env, languages));
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
                                  sub.url.spec(), sub.title, "",
                                  sub.languages));
      }
    }
  }
  return jobjects;
}

}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockSubscriptionConfig_GetRecommendedSubscriptions(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto list = adblock::config::GetKnownSubscriptions();
  return ToJavaArrayOfObjects(env, CSubscriptionsToJObjects(env, list));
}