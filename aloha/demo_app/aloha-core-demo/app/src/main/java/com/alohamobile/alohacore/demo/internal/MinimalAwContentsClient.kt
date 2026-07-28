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

import android.graphics.Bitmap
import android.graphics.Picture
import android.net.http.SslError
import android.os.Message
import android.view.KeyEvent
import android.view.View
import org.chromium.android_webview.AwConsoleMessage
import org.chromium.android_webview.AwContentsClient
import org.chromium.android_webview.AwContentsClientBridge
import org.chromium.android_webview.AwGeolocationPermissions
import org.chromium.android_webview.AwHttpAuthHandler
import org.chromium.android_webview.AwRenderProcess
import org.chromium.android_webview.AwRenderProcessGoneDetail
import org.chromium.android_webview.AwWebResourceError
import org.chromium.android_webview.AwWebResourceRequest
import org.chromium.android_webview.JsPromptResultReceiver
import org.chromium.android_webview.JsResultReceiver
import org.chromium.android_webview.permission.AwPermissionRequest
import org.chromium.android_webview.safe_browsing.AwSafeBrowsingResponse
import org.chromium.base.Callback
import org.chromium.components.embedder_support.util.WebResourceResponseInfo
import java.security.Principal

/**
 * Minimal stub implementation of AwContentsClient for demo purposes.
 * All methods have minimal no-op or default behavior.
 */
class MinimalAwContentsClient : AwContentsClient() {

    override fun hasWebViewClient(): Boolean = true

    override fun getVisitedHistory(callback: Callback<Array<String>>?) {
        // No-op
    }

    override fun doUpdateVisitedHistory(url: String?, isReload: Boolean) {
        // No-op
    }

    override fun onProgressChanged(progress: Int) {
        // No-op
    }

    override fun shouldInterceptRequest(request: AwWebResourceRequest?): WebResourceResponseInfo? {
        return null
    }

    override fun shouldOverrideKeyEvent(event: KeyEvent?): Boolean {
        return false
    }

    override fun shouldOverrideUrlLoading(request: AwWebResourceRequest?): Boolean {
        // Return false to handle navigation inside the WebView
        // Return true only if you want to open URL in external browser
        return false
    }

    override fun onLoadResource(url: String?) {
        // No-op
    }

    override fun onUnhandledKeyEvent(event: KeyEvent?) {
        // No-op
    }

    override fun onConsoleMessage(consoleMessage: AwConsoleMessage?): Boolean {
        return false
    }

    override fun onReceivedHttpAuthRequest(
        handler: AwHttpAuthHandler?,
        host: String?,
        realm: String?,
    ) {
        handler?.cancel()
    }

    override fun onReceivedSslError(callback: Callback<Boolean>?, error: SslError?) {
        callback?.onResult(false)
    }

    override fun onReceivedClientCertRequest(
        callback: AwContentsClientBridge.ClientCertificateRequestCallback?,
        keyTypes: Array<out String>?,
        principals: Array<out Principal>?,
        host: String?,
        port: Int,
    ) {
        callback?.ignore()
    }

    override fun onReceivedLoginRequest(realm: String?, account: String?, args: String?) {
        // No-op
    }

    override fun onFormResubmission(dontResend: Message?, resend: Message?) {
        dontResend?.sendToTarget()
    }

    override fun onDownloadStart(
        url: String?,
        userAgent: String?,
        contentDisposition: String?,
        mimeType: String?,
        contentLength: Long,
    ) {
        // No-op
    }

    override fun showFileChooser(
        uploadFilePathsCallback: Callback<Array<String>>?,
        fileChooserParams: FileChooserParamsImpl?,
    ) {
        uploadFilePathsCallback?.onResult(null)
    }

    override fun onGeolocationPermissionsShowPrompt(
        origin: String?,
        callback: AwGeolocationPermissions.Callback?,
    ) {
        callback?.invoke(origin, false, false)
    }

    override fun onGeolocationPermissionsHidePrompt() {
        // No-op
    }

    override fun onPermissionRequest(awPermissionRequest: AwPermissionRequest?) {
        awPermissionRequest?.deny()
    }

    override fun onPermissionRequestCanceled(awPermissionRequest: AwPermissionRequest?) {
        // No-op
    }

    override fun onScaleChangedScaled(oldScale: Float, newScale: Float) {
        // No-op
    }

    override fun handleJsAlert(url: String?, message: String?, receiver: JsResultReceiver?) {
        receiver?.cancel()
    }

    override fun handleJsBeforeUnload(url: String?, message: String?, receiver: JsResultReceiver?) {
        receiver?.cancel()
    }

    override fun handleJsConfirm(url: String?, message: String?, receiver: JsResultReceiver?) {
        receiver?.cancel()
    }

    override fun handleJsPrompt(
        url: String?,
        message: String?,
        defaultValue: String?,
        receiver: JsPromptResultReceiver?,
    ) {
        receiver?.cancel()
    }

    override fun onCreateWindow(
        isDialog: Boolean,
        isUserGesture: Boolean,
        targetUrl: String?,
        isGoogleAuth3PCookiesRequired: Boolean,
    ): Boolean {
        return false
    }

    override fun onCloseWindow() {
        // No-op
    }

    override fun onReceivedTouchIconUrl(url: String?, precomposed: Boolean) {
        // No-op
    }

    override fun onReceivedIcon(bitmap: Bitmap?) {
        // No-op
    }

    override fun onReceivedTitle(title: String?) {
        // No-op
    }

    override fun onRequestFocus() {
        // No-op
    }

    override fun getVideoLoadingProgressView(): View? {
        return null
    }

    override fun onPageStarted(url: String?) {
        // No-op
    }

    override fun onPageFinished(url: String?) {
        // No-op
    }

    override fun onPageCommitVisible(url: String?) {
        // No-op
    }

    override fun onReceivedError(request: AwWebResourceRequest?, error: AwWebResourceError?) {
        // No-op
    }

    override fun onSafeBrowsingHit(
        request: AwWebResourceRequest?,
        threatType: Int,
        callback: Callback<AwSafeBrowsingResponse>?,
    ) {
        callback?.onResult(null)
    }

    override fun onReceivedHttpError(
        request: AwWebResourceRequest?,
        response: WebResourceResponseInfo?,
    ) {
        // No-op
    }

    override fun onShowCustomView(view: View?, callback: CustomViewCallback?) {
        // No-op
    }

    override fun onHideCustomView() {
        // No-op
    }

    override fun getDefaultVideoPoster(): Bitmap? {
        return null
    }

    override fun onFindResultReceived(
        activeMatchOrdinal: Int,
        numberOfMatches: Int,
        isDoneCounting: Boolean,
    ) {
        // No-op
    }

    override fun onNewPicture(picture: Picture?) {
        // No-op
    }

    override fun onRendererUnresponsive(renderProcess: AwRenderProcess?) {
        // No-op
    }

    override fun onRendererResponsive(renderProcess: AwRenderProcess?) {
        // No-op
    }

    override fun onRenderProcessGone(detail: AwRenderProcessGoneDetail?): Boolean {
        return true
    }

    override fun onMediaPlay(
        playerId: MediaPlayerId?,
        mediaUrl: String?,
        documentUrl: String?,
        durationSec: Double,
        isAudioOnly: Boolean,
    ) {
        // No-op
    }

    override fun onMediaPause(
        playerId: MediaPlayerId?,
        mediaUrl: String?,
        documentUrl: String?,
        durationSec: Double,
        isAudioOnly: Boolean,
    ) {
        // No-op
    }

    override fun onMediaDestroy(
        playerId: MediaPlayerId?,
        mediaUrl: String?,
        documentUrl: String?,
    ) {
        // No-op
    }

    override fun onMediaError(
        playerId: MediaPlayerId?,
        pipelineStatus: String?,
        mediaUrl: String?,
        documentUrl: String?,
        currentTimeSec: Double,
        durationSec: Double,
    ) {
        // No-op
    }

    override fun shouldHideControlsInFullscreen(
        playerId: MediaPlayerId?,
        url: String?,
        durationSec: Double,
        playerClass: String?,
    ): Boolean {
        return false
    }

    override fun onVideoEnterFullscreen(
        playerId: MediaPlayerId?,
        url: String?,
        mediaControlsIsHidden: Boolean,
    ) {
        // No-op
    }

    override fun onVideoExitFullscreen(playerId: MediaPlayerId?, url: String?) {
        // No-op
    }

    override fun onEnterFullscreen() {
        // No-op
    }

    override fun onExitFullscreen() {
        // No-op
    }

    override fun onPageLoaded(url: String?, isError: Boolean) {
        // No-op
    }

    override fun onInternalDownloadStarted(httpMethod: String?, sizeInBytes: Int) {
        // No-op
    }

    override fun onDownloadToCacheFinished(
        url: String?,
        originalUrl: String?,
        userAgent: String?,
        contentDisposition: String?,
        mimeType: String?,
        suggestedFilename: String?,
        downloadedFilePath: String?,
    ) {
        // No-op
    }

    override fun onRequestedDownloadUrl(url: String?) {
        // No-op
    }

    override fun shouldPlayBackgroundVideo(): Boolean {
        return false
    }

    override fun onHlsDetected(url: String?, requestHeaders: String?) {
        // No-op
    }

    override fun getUserAgent(): String? {
        return null
    }
}
