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

package com.alohamobile.alohacore.demo

import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import com.alohamobile.alohacore.demo.internal.AwSettingsFactory
import com.alohamobile.alohacore.demo.internal.DrawFnAccess
import com.alohamobile.alohacore.demo.internal.InternalAccessDelegateImpl
import com.alohamobile.alohacore.demo.internal.MinimalAwContentsClient
import com.alohamobile.alohacore.demo.view.SimpleAlohaCoreWebView
import org.chromium.android_webview.AwBrowserContext
import org.chromium.android_webview.AwContents

class AlohaCoreDemoActivity : AppCompatActivity() {

    private var webView: SimpleAlohaCoreWebView? = null
    private var awContents: AwContents? = null

    private val awSettingsFactory: AwSettingsFactory = AwSettingsFactory()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        setContentView(R.layout.activity_alohacore_demo)
        setupWindowInsets()

        val urlInput = findViewById<EditText>(R.id.urlInput)
        val goButton = findViewById<Button>(R.id.goButton)
        val webViewContainer = findViewById<FrameLayout>(R.id.webViewContainer)

        setupWebView(webViewContainer)

        goButton.setOnClickListener {
            val url = urlInput.text.toString().trim()
            if (url.isNotEmpty()) {
                val finalUrl = if (!url.startsWith("http://") && !url.startsWith("https://")) {
                    "https://$url"
                } else {
                    url
                }
                awContents?.loadUrl(finalUrl)
            }
        }

        awContents?.loadUrl("https://alohabrowser.com")
    }

    private fun setupWindowInsets() {
        val rootView = findViewById<FrameLayout>(R.id.main)
        val urlInput = findViewById<EditText>(R.id.urlInput)
        val webViewContainer = findViewById<FrameLayout>(R.id.webViewContainer)

        ViewCompat.setOnApplyWindowInsetsListener(rootView) { _, windowInsets ->
            val insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())

            urlInput.updatePadding(
                left = urlInput.paddingLeft + insets.left,
                top = insets.top,
                right = urlInput.paddingRight + insets.right,
            )

            webViewContainer.updatePadding(
                bottom = insets.bottom,
            )

            windowInsets
        }
    }

    private fun setupWebView(container: FrameLayout) {
        val webView = SimpleAlohaCoreWebView(this)
        this.webView = webView

        val internalAccessDelegate = InternalAccessDelegateImpl(webView)
        val awContentsClient = MinimalAwContentsClient()
        val awSettings = awSettingsFactory.createAwSettings(this)

        val awContents = AwContents(
            /* browserContext = */ AwBrowserContext.getDefault(),
            /* containerView = */ webView,
            /* context = */ this,
            /* internalAccessAdapter = */ internalAccessDelegate,
            /* drawFnAccess = */ DrawFnAccess(),
            /* contentsClient = */ awContentsClient,
            /* awSettings = */ awSettings,
        )
        awContents.setBackgroundColor(Color.WHITE)

        webView.bindTo(awContents)
        this.awContents = awContents

        container.addView(webView)
    }

    override fun onDestroy() {
        super.onDestroy()
        awContents?.destroy()
        webView?.unbind()
    }
}
