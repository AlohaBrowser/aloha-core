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

#ifndef COMPONENTS_ADBLOCK_ANDROID_RESOURCE_CLASSIFICATION_NOTIFIER_ANDROID_H_
#define COMPONENTS_ADBLOCK_ANDROID_RESOURCE_CLASSIFICATION_NOTIFIER_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "content/public/browser/browser_context.h"

class FilterMatchResult;

namespace adblock {

class ResourceClassificationNotifierAndroid
    : public ResourceClassificationRunner::Observer {
 public:
  ResourceClassificationNotifierAndroid(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcontroller,
      ResourceClassificationRunner* classification_runner);
  ~ResourceClassificationNotifierAndroid() override;

  // ResourceClassificationRunner::Observer
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override;
  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override;
  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<ResourceClassificationRunner> classification_runner_;

  // Direct reference to ResourceClassificationNotifier java class. Kept for as
  // long as this instance of ResourceClassificationNotifierAndroid lives:
  // until corresponding Profile gets destroyed. Destruction of Profile triggers
  // destruction of both C++ ResourceClassificationNotifierAndroid and Java
  // ResourceClassificationNotifier objects.
  const JavaObjectWeakGlobalRef java_weak_controller_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ANDROID_RESOURCE_CLASSIFICATION_NOTIFIER_ANDROID_H_
