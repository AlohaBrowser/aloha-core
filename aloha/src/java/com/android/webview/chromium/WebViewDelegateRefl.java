/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 // Modified by Aloha Mobile Ltd.
 
package com.android.webview.chromium;

import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.util.Log;
import android.view.View;
import android.webkit.WebViewFactory;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;

import android.content.res.AssetManager;
import android.content.pm.PackageInfo;

import com.android.webview.chromium.WebViewDelegate;

/**
 * Delegate used by the WebView provider implementation to access
 * the required framework functionality needed to implement a {@link WebView}.
 *
 * @hide
 */
public class WebViewDelegateRefl implements WebViewDelegate {
    private static final String TAG = "WebViewDelegateRefl";
    private static final long TRACE_TAG_WEBVIEW = 1L << 4;

    // private final Method mSetOnTraceEnabledChangeListener;
    private Method mCanInvokeDrawGlFunctor;
    private Method mInvokeDrawGlFunctor;
    private Method mCallDrawGlFunction2;
    private Method mCallDrawGlFunction3;
    private Method mDetachDrawGlFunctor;
    private Method mGetPackageId;
    private Method mGetApplication;
    private Method mGetErrorString;
    private Method mAddAssetPathMethod;
    private Method mDrawWebViewFunctor;
    private Method mGetStartupTimestamps;

    private final Object mDelegate;

    public WebViewDelegateRefl() {
        try{
            Constructor c = Class.forName("android.webkit.WebViewDelegate").getDeclaredConstructor();
            c.setAccessible(true);
            mDelegate = c.newInstance();

            mCanInvokeDrawGlFunctor = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("canInvokeDrawGlFunctor", View.class);
            mInvokeDrawGlFunctor = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("invokeDrawGlFunctor", View.class, long.class, boolean.class);
            mCallDrawGlFunction2 = Class.forName("android.webkit.WebViewDelegate")
                        .getMethod("callDrawGlFunction", Canvas.class, long.class);

            try {
                mCallDrawGlFunction3 = Class.forName("android.webkit.WebViewDelegate")
                        .getMethod("callDrawGlFunction", Canvas.class, long.class, Runnable.class);
            } catch(Exception e) {
                Log.w(TAG, "Can't get callDrawGlFunction3: " + e);
            }

            mDetachDrawGlFunctor = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("detachDrawGlFunctor", View.class, long.class);
            mGetPackageId = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("getPackageId", Resources.class, String.class);
            mGetApplication = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("getApplication");
            mGetErrorString = Class.forName("android.webkit.WebViewDelegate")
                    .getMethod("getErrorString", Context.class, int.class);
            mAddAssetPathMethod = AssetManager.class.getMethod("addAssetPath", String.class);

            try {
                mGetStartupTimestamps = Class.forName("android.webkit.WebViewDelegate")
                        .getMethod("getStartupTimestamps");
            } catch (Exception e) {
                Log.w(TAG, "Can't get getStartupTimestamps: " + e);
            }

            try {
                mDrawWebViewFunctor = Class.forName("android.webkit.WebViewDelegate")
                        .getMethod("drawWebViewFunctor", Canvas.class, int.class);
            } catch(Exception e) {
                Log.w(TAG, "Can't get drawWebViewFunctor: " + e);
            }

        } catch(Exception e) {
            throw new RuntimeException("Failed to get function", e);
        }
    }


    /**
     * Returns true if the draw GL functor can be invoked (see {@link #invokeDrawGlFunctor})
     * and false otherwise.
     */
    @Override
    public boolean canInvokeDrawGlFunctor(View containerView) {
        try {
            return (Boolean)mCanInvokeDrawGlFunctor.invoke(mDelegate, containerView);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Invokes the draw GL functor. If waitForCompletion is false the functor
     * may be invoked asynchronously.
     *
     * @param nativeDrawGLFunctor the pointer to the native functor that implements
     *        system/core/include/utils/Functor.h
     */
    @Override
    public void invokeDrawGlFunctor(View containerView, long nativeDrawGLFunctor,
            boolean waitForCompletion) {
        try {
            mInvokeDrawGlFunctor.invoke(mDelegate, containerView, nativeDrawGLFunctor, waitForCompletion);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Calls the function specified with the nativeDrawGLFunctor functor pointer. This
     * functionality is used by the WebView for calling into their renderer from the
     * framework display lists.
     *
     * @param canvas a hardware accelerated canvas (see {@link Canvas#isHardwareAccelerated()})
     * @param nativeDrawGLFunctor the pointer to the native functor that implements
     *        system/core/include/utils/Functor.h
     * @throws IllegalArgumentException if the canvas is not hardware accelerated
     */
    @Override
    public void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor) {
        try {
            mCallDrawGlFunction2.invoke(mDelegate, canvas, nativeDrawGLFunctor);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Calls the function specified with the nativeDrawGLFunctor functor pointer. This
     * functionality is used by the WebView for calling into their renderer from the
     * framework display lists.
     *
     * @param canvas a hardware accelerated canvas (see {@link Canvas#isHardwareAccelerated()})
     * @param nativeDrawGLFunctor the pointer to the native functor that implements
     *        system/core/include/utils/Functor.h
     * @param releasedRunnable Called when this nativeDrawGLFunctor is no longer referenced by this
     *        canvas, so is safe to be destroyed.
     * @throws IllegalArgumentException if the canvas is not hardware accelerated
     */
    @Override
    public void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor,
            Runnable releasedRunnable) {
        try {
            mCallDrawGlFunction3.invoke(mDelegate, canvas, nativeDrawGLFunctor, releasedRunnable);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Detaches the draw GL functor.
     *
     * @param nativeDrawGLFunctor the pointer to the native functor that implements
     *        system/core/include/utils/Functor.h
     */
    @Override
    public void detachDrawGlFunctor(View containerView, long nativeDrawGLFunctor) {
        try {
            mDetachDrawGlFunctor.invoke(mDelegate, containerView, nativeDrawGLFunctor);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Returns the package id of the given {@code packageName}.
     */
    @Override
    public int getPackageId(Resources resources, String packageName) {
        try {
            return (Integer)mGetPackageId.invoke(mDelegate, resources, packageName);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Returns the application which is embedding the WebView.
     */
    @Override
    public Application getApplication() {
        try {
            return (Application)mGetApplication.invoke(mDelegate);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Returns the error string for the given {@code errorCode}.
     */
    @Override
    public String getErrorString(Context context, int errorCode) {
        try {
            return (String)mGetErrorString.invoke(mDelegate, context, errorCode);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    /**
     * Adds the WebView asset path to {@link android.content.res.AssetManager}.
     */
    @Override
    public void addWebViewAssetPath(Context context) {
        try {
            PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(),0);
            // Avoid calling the ContextWrapper.getAssets() proxy
            // chain, which can return an unexpected AssetManager.
            mAddAssetPathMethod.invoke(
                    context.getResources().getAssets(), info.applicationInfo.sourceDir);
        } catch (Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    @Override
    public boolean isMultiProcessEnabled() {
        return false;
    }

    @Override
    public String getDataDirectorySuffix() {
        return null;
    }

    @Override
    public void drawWebViewFunctor(Canvas canvas, int functor) {
        try {
            mDrawWebViewFunctor.invoke(mDelegate, canvas, functor);
        } catch (Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }

    @Override
    public WebViewFactory.StartupTimestamps getStartupTimestamps() {
        try {
            return (WebViewFactory.StartupTimestamps)mGetStartupTimestamps.invoke(mDelegate);
        } catch(Exception e) {
            throw new RuntimeException("Invalid reflection", e);
        }
    }
}
