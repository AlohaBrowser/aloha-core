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

#include "components/adblock/android/filtering_configuration_android.h"

#include <iterator>
#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/ranges/algorithm.h"
#include "components/adblock/android/jni_headers/FilteringConfiguration_jni.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/configuration/persistent_filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/android/browser_context_handle.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;

using namespace adblock;

FilteringConfigurationAndroid::FilteringConfigurationAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcontroller,
    const std::string& configuration_name,
    SubscriptionService* subscription_service,
    PrefService* pref_service)
    : subscription_service_(subscription_service),
      pref_service_(pref_service),
      java_weak_controller_(env, jcontroller) {
  subscription_service_->AddObserver(this);
  auto* existing_configuration =
      subscription_service_->GetFilteringConfiguration(configuration_name);
  if (existing_configuration) {
    filtering_configuration_ptr = existing_configuration;
  } else {
    auto new_filtering_configuration =
        std::make_unique<PersistentFilteringConfiguration>(pref_service_,
                                                           configuration_name);
    filtering_configuration_ptr = new_filtering_configuration.get();
    subscription_service_->InstallFilteringConfiguration(
        std::move(new_filtering_configuration));
  }
  filtering_configuration_ptr->AddObserver(this);
}

FilteringConfigurationAndroid::~FilteringConfigurationAndroid() {
  filtering_configuration_ptr->RemoveObserver(this);
  subscription_service_->UninstallFilteringConfiguration(
      filtering_configuration_ptr->GetName());
  subscription_service_->RemoveObserver(this);
}

void FilteringConfigurationAndroid::Destroy(JNIEnv*) {
  delete this;
}

void FilteringConfigurationAndroid::OnEnabledStateChanged(
    FilteringConfiguration* config) {
  Notify(config, Java_FilteringConfiguration_enabledStateChanged);
}

void FilteringConfigurationAndroid::OnFilterListsChanged(
    FilteringConfiguration* config) {
  Notify(config, Java_FilteringConfiguration_filterListsChanged);
}

void FilteringConfigurationAndroid::OnAllowedDomainsChanged(
    FilteringConfiguration* config) {
  Notify(config, Java_FilteringConfiguration_allowedDomainsChanged);
}

void FilteringConfigurationAndroid::OnCustomFiltersChanged(
    FilteringConfiguration* config) {
  Notify(config, Java_FilteringConfiguration_customFiltersChanged);
}

void FilteringConfigurationAndroid::Notify(
    FilteringConfiguration* config,
    FilteringConfigurationAndroid::JavaEventListener event_listener_function) {
  JNIEnv* env = AttachCurrentThread();
  auto java_controller = java_weak_controller_.get(env);
  if (!java_controller.is_null()) {
    event_listener_function(env, java_controller);
  }
}

void FilteringConfigurationAndroid::OnSubscriptionInstalled(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  auto java_controller = java_weak_controller_.get(env);
  if (!java_controller.is_null()) {
    Java_FilteringConfiguration_onSubscriptionUpdated(env, java_controller,
                                                      j_url);
  }
}

jboolean FilteringConfigurationAndroid::IsEnabled(JNIEnv* env) const {
  return filtering_configuration_ptr->IsEnabled() ? JNI_TRUE : JNI_FALSE;
}

void FilteringConfigurationAndroid::SetEnabled(JNIEnv* env,
                                               jboolean j_enabled) {
  filtering_configuration_ptr->SetEnabled(j_enabled == JNI_TRUE);
}

void FilteringConfigurationAndroid::AddAllowedDomain(
    JNIEnv* env,
    const JavaParamRef<jstring>& allowed_domain) {
  filtering_configuration_ptr->AddAllowedDomain(
      ConvertJavaStringToUTF8(allowed_domain));
}

void FilteringConfigurationAndroid::RemoveAllowedDomain(
    JNIEnv* env,
    const JavaParamRef<jstring>& allowed_domain) {
  filtering_configuration_ptr->RemoveAllowedDomain(
      ConvertJavaStringToUTF8(allowed_domain));
}

ScopedJavaLocalRef<jobjectArray>
FilteringConfigurationAndroid::GetAllowedDomains(JNIEnv* env) const {
  return ToJavaArrayOfStrings(env,
                              filtering_configuration_ptr->GetAllowedDomains());
}

void FilteringConfigurationAndroid::AddCustomFilter(
    JNIEnv* env,
    const JavaParamRef<jstring>& custom_filter) {
  filtering_configuration_ptr->AddCustomFilter(
      ConvertJavaStringToUTF8(custom_filter));
}

void FilteringConfigurationAndroid::RemoveCustomFilter(
    JNIEnv* env,
    const JavaParamRef<jstring>& custom_filter) {
  filtering_configuration_ptr->RemoveCustomFilter(
      ConvertJavaStringToUTF8(custom_filter));
}

ScopedJavaLocalRef<jobjectArray>
FilteringConfigurationAndroid::GetCustomFilters(JNIEnv* env) const {
  return ToJavaArrayOfStrings(env,
                              filtering_configuration_ptr->GetCustomFilters());
}

void FilteringConfigurationAndroid::AddFilterList(
    JNIEnv* env,
    const JavaParamRef<jstring>& url) {
  filtering_configuration_ptr->AddFilterList(
      GURL{ConvertJavaStringToUTF8(url)});
}

void FilteringConfigurationAndroid::RemoveFilterList(
    JNIEnv* env,
    const JavaParamRef<jstring>& url) {
  filtering_configuration_ptr->RemoveFilterList(
      GURL{ConvertJavaStringToUTF8(url)});
}

ScopedJavaLocalRef<jobjectArray> FilteringConfigurationAndroid::GetFilterLists(
    JNIEnv* env) const {
  // For simplicity, convert GURL to std::string, pass to Java, and convert
  // from String to URL. Strings are easier to pass through JNI.
  std::vector<std::string> urls;
  base::ranges::transform(filtering_configuration_ptr->GetFilterLists(),
                          std::back_inserter(urls), &GURL::spec);
  return ToJavaArrayOfStrings(env, urls);
}

static ScopedJavaLocalRef<jstring>
JNI_FilteringConfiguration_GetAcceptableAdsUrl(JNIEnv* env) {
  return ConvertUTF8ToJavaString(env, adblock::AcceptableAdsUrl().spec());
}

static jlong JNI_FilteringConfiguration_Create(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcontroller,
    const JavaParamRef<jstring>& jstring,
    const JavaParamRef<jobject>& jbrowser_context_handle) {
  DCHECK(!jcontroller.is_null());
  DCHECK(!jbrowser_context_handle.is_null());
  auto* context =
      content::BrowserContextFromJavaHandle(jbrowser_context_handle);
  return reinterpret_cast<jlong>(new FilteringConfigurationAndroid(
      env, std::move(jcontroller), ConvertJavaStringToUTF8(jstring),
      adblock::SubscriptionServiceFactory::GetForBrowserContext(context),
      user_prefs::UserPrefs::Get(context)));
}

static ScopedJavaLocalRef<jobjectArray>
JNI_FilteringConfiguration_GetConfigurations(
    JNIEnv* env,
    const JavaParamRef<jobject>& jbrowser_context_handle) {
  std::vector<std::string> configurations;
  base::ranges::transform(
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          content::BrowserContextFromJavaHandle(jbrowser_context_handle))
          ->GetInstalledFilteringConfigurations(),
      std::back_inserter(configurations),
      [](adblock::FilteringConfiguration* fc) { return fc->GetName(); });
  return ToJavaArrayOfStrings(env, configurations);
}

static jboolean JNI_FilteringConfiguration_IsAutoInstallEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& jbrowser_context_handle) {
  return adblock::SubscriptionServiceFactory::GetForBrowserContext(
             content::BrowserContextFromJavaHandle(jbrowser_context_handle))
                 ->IsAutoInstallEnabled()
             ? JNI_TRUE
             : JNI_FALSE;
}

static void JNI_FilteringConfiguration_SetAutoInstallEnabled(
    JNIEnv* env,
    jboolean j_enabled,
    const JavaParamRef<jobject>& jbrowser_context_handle) {
  adblock::SubscriptionServiceFactory::GetForBrowserContext(
      content::BrowserContextFromJavaHandle(jbrowser_context_handle))
      ->SetAutoInstallEnabled(j_enabled == JNI_TRUE);
}
