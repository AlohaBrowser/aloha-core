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

#include "content/shell/android/adblock/shell_browser_context_android.h"

#include "base/android/jni_android.h"
#include "content/shell/android/content_shell_jni_headers/ShellBrowserContext_jni.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/browser/shell_content_browser_client.h"

namespace adblock {

ShellBrowserContextAndroid::ShellBrowserContextAndroid(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcontroller)
    : java_weak_controller_(env, jcontroller) {}

ShellBrowserContextAndroid::~ShellBrowserContextAndroid() = default;

static base::android::ScopedJavaLocalRef<jobject>
JNI_ShellBrowserContext_GetDefaultJava(JNIEnv* env) {
  content::ShellBrowserContext* default_context =
      content::ShellContentBrowserClient::Get()->browser_context();
  CHECK(default_context);

  return Java_ShellBrowserContext_create(
      env, reinterpret_cast<intptr_t>(default_context));
}

}  // namespace adblock
