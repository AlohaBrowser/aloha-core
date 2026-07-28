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

package com.alohamobile.alohacore.demo.view

import android.annotation.SuppressLint
import android.content.Context
import android.content.res.Configuration
import android.graphics.Canvas
import android.graphics.Rect
import android.os.Bundle
import android.util.SparseArray
import android.view.DragEvent
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewStructure
import android.view.accessibility.AccessibilityNodeProvider
import android.view.autofill.AutofillValue
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.widget.ScrollView
import org.chromium.android_webview.AwContents
import org.chromium.android_webview.AwViewMethods

/**
 * Simplified AlohaCore-based WebView for demo purposes.
 *
 * This class provides a container view that wraps [org.chromium.android_webview.AwContents] and delegates
 * all view-related operations to Chromium's Android WebView implementation.
 * It extends [android.widget.ScrollView] to provide native scrolling behavior while maintaining
 * compatibility with the Chromium rendering engine.
 *
 * @param context The Android context used to create the view.
 */
@Suppress("FunctionName")
class SimpleAlohaCoreWebView(context: Context) : ScrollView(context) {

    companion object {
        /**
         * Default over-scroll mode for the WebView.
         * Set to [android.view.View.OVER_SCROLL_IF_CONTENT_SCROLLS] to enable over-scroll effect
         * only when content is larger than the viewport.
         */
        const val DEFAULT_OVER_SCROLL_MODE = OVER_SCROLL_IF_CONTENT_SCROLLS
    }

    private var awContents: AwContents? = null

    private val awViewMethods: AwViewMethods?
        get() = awContents?.viewMethods

    init {
        defaultFocusHighlightEnabled = false
        overScrollMode = DEFAULT_OVER_SCROLL_MODE
        isHorizontalScrollBarEnabled = true
        isVerticalScrollBarEnabled = true
        isFocusable = true
        isFocusableInTouchMode = true
    }

    /**
     * Binds the given [AwContents] instance to this view.
     *
     * After binding, this view will delegate all view operations to the [AwContents]
     * instance, enabling Chromium-based web content rendering.
     *
     * @param awContents The Chromium WebView contents to bind to this view.
     */
    fun bindTo(awContents: AwContents) {
        this.awContents = awContents
    }

    /**
     * Unbinds the currently attached [AwContents] from this view.
     *
     * After unbinding, this view will no longer delegate operations to any
     * [AwContents] instance. Should be called when the WebView is being destroyed
     * or when switching to a different [AwContents] instance.
     */
    fun unbind() {
        this.awContents = null
    }

    /**
     * Sets the over-scroll mode for both this view and the bound [AwContents].
     *
     * @param overScrollMode The over-scroll mode to set. One of:
     *   - [android.view.View.OVER_SCROLL_ALWAYS]
     *   - [android.view.View.OVER_SCROLL_IF_CONTENT_SCROLLS]
     *   - [android.view.View.OVER_SCROLL_NEVER]
     */
    override fun setOverScrollMode(overScrollMode: Int) {
        super.setOverScrollMode(overScrollMode)
        awContents?.setOverScrollMode(overScrollMode)
    }

    // region AwContents.InternalAccessDelegate implementation's proxy

    /**
     * Proxy method to invoke the superclass [onKeyUp] implementation.
     *
     * Used by [AwContents.InternalAccessDelegate] to access protected View methods.
     *
     * @param keyCode The key code of the released key.
     * @param event The key event describing the release.
     * @return `true` if the event was handled by the superclass.
     */
    fun super_onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        return super.onKeyUp(keyCode, event)
    }

    /**
     * Proxy method to invoke the superclass [dispatchKeyEvent] implementation.
     *
     * Used by [AwContents.InternalAccessDelegate] to access protected View methods.
     *
     * @param event The key event to dispatch.
     * @return `true` if the event was handled by the superclass.
     */
    fun super_dispatchKeyEvent(event: KeyEvent): Boolean {
        return super.dispatchKeyEvent(event)
    }

    /**
     * Proxy method to invoke the superclass [onGenericMotionEvent] implementation.
     *
     * Used by [AwContents.InternalAccessDelegate] to access protected View methods.
     *
     * @param event The generic motion event to handle.
     * @return `true` if the event was handled by the superclass.
     */
    fun super_onGenericMotionEvent(event: MotionEvent): Boolean {
        return super.onGenericMotionEvent(event)
    }

    /**
     * Proxy method to invoke the protected [overScrollBy] method.
     *
     * Used by [AwContents.InternalAccessDelegate] to trigger over-scroll behavior.
     *
     * @param deltaX Horizontal scroll delta in pixels.
     * @param deltaY Vertical scroll delta in pixels.
     * @param scrollX Current horizontal scroll position.
     * @param scrollY Current vertical scroll position.
     * @param scrollRangeX Maximum horizontal scroll range.
     * @param scrollRangeY Maximum vertical scroll range.
     * @param maxOverScrollX Maximum horizontal over-scroll distance.
     * @param maxOverScrollY Maximum vertical over-scroll distance.
     * @param isTouchEvent `true` if the over-scroll is caused by a touch event.
     */
    fun protected_overScrollBy(
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
        this.overScrollBy(
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

    /**
     * Proxy method to invoke the superclass [scrollTo] implementation.
     *
     * Used by [AwContents.InternalAccessDelegate] to scroll the view directly.
     *
     * @param scrollX The x position to scroll to.
     * @param scrollY The y position to scroll to.
     */
    fun super_scrollTo(scrollX: Int, scrollY: Int) {
        super.scrollTo(scrollX, scrollY)
    }

    /**
     * Proxy method to invoke the protected [setMeasuredDimension] method.
     *
     * Used by [AwContents.InternalAccessDelegate] to set the measured dimensions
     * during the measure pass.
     *
     * @param measuredWidth The measured width of this view.
     * @param measuredHeight The measured height of this view.
     */
    fun protected_setMeasuredDimension(measuredWidth: Int, measuredHeight: Int) {
        this.setMeasuredDimension(measuredWidth, measuredHeight)
    }

    /**
     * Proxy method to invoke the superclass [onConfigurationChanged] implementation.
     *
     * Used by [AwContents.InternalAccessDelegate] to notify the superclass
     * of configuration changes.
     *
     * @param newConfig The new device configuration.
     */
    fun super_onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
    }

    // endregion

    // region Android View API

    /**
     * Draws the WebView content to the canvas.
     *
     * Delegates drawing to [AwContents] to render the web content.
     */
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        awContents?.viewMethods?.onDraw(canvas)
    }

    /**
     * Called when the view is attached to a window.
     *
     * Notifies [AwContents] that the view is now visible and can start rendering.
     */
    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        awViewMethods?.onAttachedToWindow()
    }

    /**
     * Called when the view is detached from its window.
     *
     * Notifies [AwContents] to pause rendering and release window-related resources.
     */
    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        awViewMethods?.onDetachedFromWindow()
    }

    /**
     * Called when the view is temporarily detached from its parent.
     *
     * Used during view recycling to notify [AwContents] of temporary detachment.
     */
    override fun onStartTemporaryDetach() {
        super.onStartTemporaryDetach()
        awViewMethods?.onStartTemporaryDetach()
    }

    /**
     * Called when the view is re-attached after a temporary detachment.
     *
     * Notifies [AwContents] that the view is visible again after recycling.
     */
    override fun onFinishTemporaryDetach() {
        super.onFinishTemporaryDetach()
        awViewMethods?.onFinishTemporaryDetach()
    }

    /**
     * Provides the virtual view structure for accessibility and content capture.
     *
     * Delegates to [AwContents] to populate the structure with web content information.
     */
    override fun onProvideVirtualStructure(structure: ViewStructure?) {
        awContents?.onProvideVirtualStructure(structure)
    }

    /**
     * Dispatches a key event to the WebView.
     *
     * First attempts to handle the event through [AwContents], falling back
     * to the superclass implementation if not consumed.
     *
     * @return `true` if the event was handled.
     */
    override fun dispatchKeyEvent(event: KeyEvent?): Boolean {
        if (awViewMethods?.dispatchKeyEvent(event) == true) {
            return true
        }
        return super.dispatchKeyEvent(event)
    }

    /**
     * Provides the virtual view structure for autofill purposes.
     *
     * Delegates to [AwContents] to populate the structure with form field information.
     */
    override fun onProvideAutofillVirtualStructure(structure: ViewStructure?, flags: Int) {
        awContents?.onProvideAutoFillVirtualStructure(structure, flags)
    }

    /**
     * Applies autofill values to the WebView's form fields.
     *
     * Delegates to [AwContents] to fill in the form fields with provided values.
     * Exceptions are caught and logged to prevent crashes from autofill issues.
     */
    override fun autofill(values: SparseArray<AutofillValue>) {
        super.autofill(values)
        try {
            awContents?.autofill(values)
        } catch (t: Throwable) {
            t.printStackTrace()
        }
    }

    /**
     * Called when the visibility of this view or an ancestor has changed.
     *
     * Notifies [AwContents] of visibility changes to optimize rendering.
     */
    override fun onVisibilityChanged(changedView: View, visibility: Int) {
        super.onVisibilityChanged(changedView, visibility)
        awViewMethods?.onVisibilityChanged(changedView, visibility)
    }

    /**
     * Called when the window containing this view gains or loses visibility.
     *
     * Notifies [AwContents] to pause or resume rendering based on window visibility.
     */
    override fun onWindowVisibilityChanged(visibility: Int) {
        super.onWindowVisibilityChanged(visibility)
        awViewMethods?.onWindowVisibilityChanged(visibility)
    }

    /**
     * Called when the view gains or loses focus.
     *
     * Notifies [AwContents] of focus changes for proper input handling.
     */
    public override fun onFocusChanged(focused: Boolean, direction: Int, previouslyFocusedRect: Rect?) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect)
        awViewMethods?.onFocusChanged(focused, direction, previouslyFocusedRect)
    }

    /**
     * Called when the window containing this view gains or loses focus.
     *
     * Notifies [AwContents] of window focus changes.
     */
    override fun onWindowFocusChanged(hasWindowFocus: Boolean) {
        super.onWindowFocusChanged(hasWindowFocus)
        awViewMethods?.onWindowFocusChanged(hasWindowFocus)
    }

    /**
     * Measures the view and its content to determine the measured width and height.
     *
     * Delegates to [AwContents] to properly measure web content dimensions.
     */
    public override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        awViewMethods?.onMeasure(widthMeasureSpec, heightMeasureSpec)
    }

    /**
     * Called when the size of this view has changed.
     *
     * Notifies [AwContents] of size changes to update the viewport.
     */
    public override fun onSizeChanged(w: Int, h: Int, ow: Int, oh: Int) {
        super.onSizeChanged(w, h, ow, oh)
        awViewMethods?.onSizeChanged(w, h, ow, oh)
    }

    /**
     * Called when the device configuration changes.
     *
     * Notifies [AwContents] of configuration changes (e.g., orientation, locale).
     */
    override fun onConfigurationChanged(newConfig: Configuration?) {
        super.onConfigurationChanged(newConfig)
        awViewMethods?.onConfigurationChanged(newConfig)
    }

    /**
     * Handles key release events.
     *
     * First attempts to handle the event through [AwContents], falling back
     * to the superclass implementation if not consumed.
     *
     * @return `true` if the event was handled.
     */
    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {
        if (awViewMethods?.onKeyUp(keyCode, event) == true) {
            return true
        }
        return super.onKeyUp(keyCode, event)
    }

    /**
     * Handles drag and drop events.
     *
     * Delegates drag events to [AwContents] for web content drag-and-drop support.
     *
     * @return `true` if the event was handled.
     */
    override fun onDragEvent(event: DragEvent?): Boolean {
        if (awViewMethods?.onDragEvent(event) == true) {
            return true
        }
        return super.onDragEvent(event)
    }

    /**
     * Handles touch events.
     *
     * All touch events are delegated to [AwContents] for gesture handling,
     * scrolling, and interaction with web content.
     *
     * @return `true` if the event was handled by [AwContents].
     */
    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(ev: MotionEvent): Boolean {
        return awViewMethods?.onTouchEvent(ev) == true
    }

    /**
     * Handles hover events from pointing devices.
     *
     * Delegates hover events to [AwContents] for web content hover states.
     *
     * @return `true` if the event was handled.
     */
    override fun onHoverEvent(ev: MotionEvent): Boolean {
        if (awViewMethods?.onHoverEvent(ev) == true) {
            return true
        }
        return super.onHoverEvent(ev)
    }

    /**
     * Handles generic motion events (e.g., joystick, mouse scroll).
     *
     * Delegates motion events to [AwContents] for handling.
     *
     * @return `true` if the event was handled.
     */
    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (awViewMethods?.onGenericMotionEvent(event) == true) {
            return true
        }
        return super.onGenericMotionEvent(event)
    }

    /**
     * Returns the accessibility node provider for this view.
     *
     * Delegates to [AwContents] to provide accessibility information about web content.
     */
    override fun getAccessibilityNodeProvider(): AccessibilityNodeProvider? {
        return awViewMethods?.accessibilityNodeProvider ?: super.getAccessibilityNodeProvider()
    }

    /**
     * Performs an accessibility action on the view.
     *
     * Delegates accessibility actions to [AwContents] for web content interaction.
     *
     * @return `true` if the action was handled.
     */
    override fun performAccessibilityAction(action: Int, arguments: Bundle?): Boolean {
        if (arguments != null && awViewMethods?.performAccessibilityAction(action, arguments) == true) {
            return true
        }
        return super.performAccessibilityAction(action, arguments)
    }

    /**
     * Creates an input connection for text input.
     *
     * Delegates to [AwContents] to create an input connection for web form fields.
     *
     * @return An [android.view.inputmethod.InputConnection] for text input, or `null` if not applicable.
     */
    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection? {
        return awViewMethods?.onCreateInputConnection(outAttrs)
    }

    /**
     * Called to perform scrolling animations.
     *
     * Delegates to [AwContents] to handle fling and smooth scroll animations.
     */
    override fun computeScroll() {
        super.computeScroll()
        awViewMethods?.computeScroll()
    }

    /**
     * Called when the view has been scrolled.
     *
     * Notifies [AwContents] of over-scroll state changes.
     */
    override fun onOverScrolled(scrollX: Int, scrollY: Int, clampedX: Boolean, clampedY: Boolean) {
        super.onOverScrolled(scrollX, scrollY, clampedX, clampedY)
        awViewMethods?.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY)
    }

    /**
     * Called when the scroll position of the view changes.
     *
     * Notifies [AwContents] of scroll position changes for proper content positioning.
     */
    override fun onScrollChanged(l: Int, t: Int, oldl: Int, oldt: Int) {
        super.onScrollChanged(l, t, oldl, oldt)
        awViewMethods?.onContainerViewScrollChanged(l, t, oldl, oldt)
    }

    /**
     * Called during layout to position child views.
     *
     * This is intentionally empty because WebView renders its content through
     * [AwContents] rather than child views.
     */
    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        // WebView has no nested views, we shouldn't lay out anything
    }

    /**
     * Checks if this view is a text editor.
     *
     * Delegates to [AwContents] to determine if a text input field is focused.
     *
     * @return `true` if currently editing text in web content.
     */
    override fun onCheckIsTextEditor(): Boolean {
        return runCatching { awViewMethods?.onCheckIsTextEditor() }.getOrNull() ?: false
    }

    /**
     * Computes the horizontal scroll range of the web content.
     *
     * @return The total horizontal scrollable range in pixels.
     */
    override fun computeHorizontalScrollRange(): Int {
        return awViewMethods?.computeHorizontalScrollRange() ?: 0
    }

    /**
     * Computes the current horizontal scroll offset.
     *
     * @return The current horizontal scroll position in pixels.
     */
    override fun computeHorizontalScrollOffset(): Int {
        return awViewMethods?.computeHorizontalScrollOffset() ?: 0
    }

    /**
     * Computes the vertical scroll range of the web content.
     *
     * @return The total vertical scrollable range in pixels.
     */
    override fun computeVerticalScrollRange(): Int {
        return awViewMethods?.computeVerticalScrollRange() ?: 0
    }

    /**
     * Computes the current vertical scroll offset.
     *
     * @return The current vertical scroll position in pixels.
     */
    override fun computeVerticalScrollOffset(): Int {
        return awViewMethods?.computeVerticalScrollOffset() ?: 0
    }

    /**
     * Computes the vertical scroll extent (visible portion of content).
     *
     * @return The vertical extent of the visible content in pixels.
     */
    override fun computeVerticalScrollExtent(): Int {
        return awViewMethods?.computeVerticalScrollExtent() ?: 0
    }

    // endregion
}
