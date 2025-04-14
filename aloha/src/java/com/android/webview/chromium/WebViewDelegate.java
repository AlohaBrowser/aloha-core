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
package com.android.webview.chromium;

import android.view.View;
import android.graphics.Canvas;
import android.content.Context;
import android.content.res.Resources;
import android.app.Application;
import android.webkit.WebViewFactory;
import org.chromium.android_webview.gfx.AwDrawFnImpl;

public interface WebViewDelegate extends AwDrawFnImpl.DrawFnAccess {
    void detachDrawGlFunctor(View containerView, long nativeDrawGLFunctor);
    void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor);
    void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor, Runnable releasedRunnable);
    boolean canInvokeDrawGlFunctor(View containerView);
    void invokeDrawGlFunctor(View containerView, long nativeDrawGLFunctor, boolean waitForCompletion);
    void addWebViewAssetPath(Context context);
    Application getApplication();
    String getErrorString(Context context, int errorCode);
    int getPackageId(Resources resources, String packageName);
    boolean isMultiProcessEnabled();
    String getDataDirectorySuffix();

    @Override
    void drawWebViewFunctor(Canvas canvas, int functor);

    WebViewFactory.StartupTimestamps getStartupTimestamps();
}
