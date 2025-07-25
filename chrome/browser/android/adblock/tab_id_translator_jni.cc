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

#include "chrome/android/chrome_jni_headers/TabIdTranslator_jni.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/web_contents.h"

namespace {
constexpr int kNoTabId = -1;
}

static jint JNI_TabIdTranslator_FromRenderFrameHostId(JNIEnv* env,
                                                      jint render_process_id,
                                                      jint render_frame_id) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  if (!web_contents) {
    return kNoTabId;
  }

  auto* tab = TabAndroid::FromWebContents(web_contents);
  if (!tab) {
    return kNoTabId;
  }

  return tab->GetAndroidId();
}
