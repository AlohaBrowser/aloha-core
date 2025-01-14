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

#ifndef COMPONENTS_ADBLOCK_ANDROID_FILTERING_CONFIGURATION_ANDROID_H_
#define COMPONENTS_ADBLOCK_ANDROID_FILTERING_CONFIGURATION_ANDROID_H_

#include <map>
#include <utility>
#include <vector>
#include "base/android/jni_weak_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"

class FilteringConfigurationAndroid
    : public adblock::FilteringConfiguration::Observer,
      public adblock::SubscriptionService::SubscriptionObserver {
 public:
  explicit FilteringConfigurationAndroid(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcontroller,
      const std::string& configuration_name,
      adblock::SubscriptionService* subscription_service,
      PrefService* pref_service);
  ~FilteringConfigurationAndroid() override;
  // Called by Java to destroy this instance.
  void Destroy(JNIEnv*);
  void SetEnabled(JNIEnv* env, jboolean j_enabled);
  jboolean IsEnabled(JNIEnv* env) const;
  void AddFilterList(JNIEnv* env,
                     const base::android::JavaParamRef<jstring>& url);
  void RemoveFilterList(JNIEnv* env,
                        const base::android::JavaParamRef<jstring>& url);
  base::android::ScopedJavaLocalRef<jobjectArray> GetFilterLists(
      JNIEnv* env) const;
  void AddAllowedDomain(JNIEnv* env,
                        const base::android::JavaParamRef<jstring>& domain);
  void RemoveAllowedDomain(JNIEnv* env,
                           const base::android::JavaParamRef<jstring>& domain);
  base::android::ScopedJavaLocalRef<jobjectArray> GetAllowedDomains(
      JNIEnv* env) const;
  void AddCustomFilter(JNIEnv* env,
                       const base::android::JavaParamRef<jstring>& filter);
  void RemoveCustomFilter(JNIEnv* env,
                          const base::android::JavaParamRef<jstring>& filter);
  base::android::ScopedJavaLocalRef<jobjectArray> GetCustomFilters(
      JNIEnv* env) const;

  // FilteringConfiguration::Observer:
  void OnEnabledStateChanged(adblock::FilteringConfiguration* config) override;
  void OnFilterListsChanged(adblock::FilteringConfiguration* config) override;
  void OnAllowedDomainsChanged(
      adblock::FilteringConfiguration* config) override;
  void OnCustomFiltersChanged(adblock::FilteringConfiguration* config) override;

  // SubcriptionService::Observer:
  void OnSubscriptionInstalled(const GURL& subscription_url) override;

 private:
  using JavaEventListener = void(JNIEnv* env,
                                 const base::android::JavaRef<jobject>& obj);
  void Notify(adblock::FilteringConfiguration* config,
              JavaEventListener event_listener_function);
  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<adblock::SubscriptionService> subscription_service_;
  raw_ptr<PrefService> pref_service_;
  raw_ptr<adblock::FilteringConfiguration> filtering_configuration_ptr;

  // Direct reference to FilteringConfiguration java class. Kept for as
  // long as this instance of FilteringConfigurationAndroid lives:
  // until corresponding Profile gets destroyed. Destruction of Profile triggers
  // destruction of both C++ FilteringConfigurationAndroid and Java
  // FilteringConfiguration objects.
  const JavaObjectWeakGlobalRef java_weak_controller_;
};

#endif  // COMPONENTS_ADBLOCK_ANDROID_FILTERING_CONFIGURATION_ANDROID_H_
