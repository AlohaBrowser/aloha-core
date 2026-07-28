// Copyright 2026 Aloha Mobile Ltd.

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

package com.alohamobile.alohacore.demo.internal

import android.content.Intent
import android.content.res.Configuration
import android.view.KeyEvent
import android.view.MotionEvent
import com.alohamobile.alohacore.demo.view.SimpleAlohaCoreWebView
import org.chromium.android_webview.AwContents

internal class InternalAccessDelegateImpl(
    private val webView: SimpleAlohaCoreWebView,
) : AwContents.InternalAccessDelegate {

    override fun super_onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        return webView.super_onKeyUp(keyCode, event)
    }

    override fun super_dispatchKeyEvent(event: KeyEvent): Boolean {
        return webView.super_dispatchKeyEvent(event)
    }

    override fun super_onGenericMotionEvent(event: MotionEvent): Boolean {
        return webView.super_onGenericMotionEvent(event)
    }

    override fun onScrollChanged(l: Int, t: Int, oldl: Int, oldt: Int) {
        // Intentional no-op
    }

    override fun overScrollBy(
        deltaX: Int,
        deltaY: Int,
        scrollX: Int,
        scrollY: Int,
        scrollRangeX: Int,
        scrollRangeY: Int,
        maxOverScrollX: Int,
        maxOverScrollY: Int,
        isTouchEvent: Boolean,
    ) {
        webView.protected_overScrollBy(
            deltaX,
            deltaY,
            scrollX,
            scrollY,
            scrollRangeX,
            scrollRangeY,
            maxOverScrollX,
            maxOverScrollY,
            isTouchEvent,
        )
    }

    override fun super_scrollTo(scrollX: Int, scrollY: Int) {
        webView.super_scrollTo(scrollX, scrollY)
    }

    override fun setMeasuredDimension(measuredWidth: Int, measuredHeight: Int) {
        webView.protected_setMeasuredDimension(measuredWidth, measuredHeight)
    }

    override fun super_getScrollBarStyle(): Int {
        return webView.scrollBarStyle
    }

    override fun super_startActivityForResult(intent: Intent, requestCode: Int) {
        // No-op for demo
    }

    override fun super_onConfigurationChanged(newConfig: Configuration) {
        webView.super_onConfigurationChanged(newConfig)
    }
}
