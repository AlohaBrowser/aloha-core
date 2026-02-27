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

package com.alohamobile.bromium.webid;

import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

/**
 * JNI bridge between the C++ AlohaIdentityDialogController and the Android UI layer.
 *
 * <p>The Kotlin/Java UI layer must subclass this and register a {@link Factory} via
 * {@link #setFactory(Factory)} at application startup (before any WebView loads a
 * page that uses FedCM). The factory is called each time a FedCM request is initiated.
 *
 * <p>UI callbacks back to the engine are delivered through the protected helper
 * methods: {@link #onAccountSelected}, {@link #onDismiss}, {@link #onLoginToIdP},
 * {@link #onModalDialogClosed}.
 */
@JNINamespace("aloha")
public abstract class FedCmAccountSelectionBridge {

    // -------------------------------------------------------------------------
    // Factory — registered once by the app layer
    // -------------------------------------------------------------------------

    /** Factory that creates concrete UI implementations per FedCM request. */
    public interface Factory {
        /**
         * Called on the UI thread when a FedCM dialog is required.
         *
         * @param webContents  The WebContents that triggered the FedCM request.
         * @param window       The WindowAndroid associated with the WebContents.
         * @param rpMode       blink::mojom::RpMode: 0 = passive, 1 = active.
         * @return A new bridge instance, or null to cancel the request.
         */
        FedCmAccountSelectionBridge create(WebContents webContents,
                WindowAndroid window, int rpMode);
    }

    private static Factory sFactory;

    /** Register the UI factory. Call this once during application initialisation. */
    public static void setFactory(Factory factory) {
        sFactory = factory;
    }

    // -------------------------------------------------------------------------
    // Lifecycle — called from C++
    // -------------------------------------------------------------------------

    /** Pointer to the native AlohaIdentityDialogController. 0 after destroy(). */
    private long mNativeController;

    /**
     * Called by C++ to create a bridge instance. Returns null when no factory
     * is registered, causing the FedCM request to be silently cancelled.
     */
    @CalledByNative
    private static FedCmAccountSelectionBridge createFromNative(
            long nativeController, WebContents webContents,
            WindowAndroid windowAndroid, int rpMode) {
        if (sFactory == null) return null;
        FedCmAccountSelectionBridge bridge =
                sFactory.create(webContents, windowAndroid, rpMode);
        if (bridge != null) {
            bridge.mNativeController = nativeController;
        }
        return bridge;
    }

    /** Called by C++ destructor. Clears the native pointer. */
    @CalledByNative
    public void destroy() {
        mNativeController = 0;
    }

    // -------------------------------------------------------------------------
    // Dialog show methods — implemented by the UI layer
    // -------------------------------------------------------------------------

    /**
     * Show the main account-chooser sheet.
     *
     * @param rpData     Relying party information (origin, icon, etc.).
     * @param accounts   All accounts available for selection.
     * @param idps       Identity providers participating in this request.
     * @param newAccounts Accounts that have just signed in (highlighted).
     * @return true if the dialog was shown; false to cancel the flow.
     */
    @CalledByNative
    public abstract boolean showAccounts(FedCmRelyingPartyData rpData,
            FedCmAccount[] accounts,
            FedCmIdentityProviderData[] idps,
            FedCmAccount[] newAccounts);

    /**
     * Show a failure dialog when the IdP claims the user is signed in but
     * returns no accounts. The user can tap "Continue" to navigate to the IdP.
     */
    @CalledByNative
    public abstract boolean showFailureDialog(FedCmRelyingPartyData rpData,
            String idpForDisplay,
            FedCmIdentityProviderMetadata idpMetadata,
            int rpContext);

    /**
     * Show an error dialog after the IdP returned a token-level error.
     *
     * @param tokenError Always non-null. If the IdP did not provide error details,
     *                   both {@link FedCmTokenError#getCode()} and
     *                   {@link FedCmTokenError#getUrl()} return empty strings.
     */
    @CalledByNative
    public abstract boolean showErrorDialog(FedCmRelyingPartyData rpData,
            String idpForDisplay,
            FedCmIdentityProviderMetadata idpMetadata,
            int rpContext,
            FedCmTokenError tokenError);

    /**
     * Show a loading/spinner sheet during the active (button-triggered) mode
     * while accounts are being fetched.
     */
    @CalledByNative
    public abstract boolean showLoadingDialog(FedCmRelyingPartyData rpData,
            String idpForDisplay,
            int rpContext);

    /**
     * Show a "verifying…" sheet after the user selects an account, while the
     * engine waits for the IdP token response.
     *
     * @param isAutoReauthn true when the browser auto-selected the account
     *                      (silent re-authentication).
     */
    @CalledByNative
    public abstract boolean showVerifyingDialog(FedCmRelyingPartyData rpData,
            FedCmAccount account,
            boolean isAutoReauthn);

    /**
     * Open a modal dialog (embedded WebView or custom tab) pointing to the
     * IdP login URL.
     *
     * <p>The default implementation does nothing and returns null, which is
     * acceptable for a first integration — IdP flows that require a popup will
     * fail gracefully.
     *
     * @return The WebContents of the opened dialog, or null.
     */
    @CalledByNative
    public WebContents showModalDialog(String url) {
        return null;
    }

    /** Close any modal dialog opened by {@link #showModalDialog}. */
    @CalledByNative
    public void closeModalDialog() {}

    /**
     * When called on a bridge instance that lives inside a modal dialog, returns
     * the WebContents of the original RP page that opened the dialog.
     */
    @CalledByNative
    public WebContents getRpWebContents() {
        return null;
    }

    // -------------------------------------------------------------------------
    // Callbacks to C++ — called by the UI layer
    // -------------------------------------------------------------------------

    /**
     * Call when the user selects an account.
     *
     * @param idpConfigUrl The config_url of the IdP that owns the selected account.
     * @param accountId    The account id as returned by the IdP accounts endpoint.
     * @param isSignIn     true for a returning user (sign-in), false for sign-up.
     */
    protected final void onAccountSelected(String idpConfigUrl, String accountId,
            boolean isSignIn) {
        if (mNativeController != 0) {
            FedCmAccountSelectionBridgeJni.get().onAccountSelected(
                    mNativeController, idpConfigUrl, accountId, isSignIn);
        }
    }

    /**
     * Call when the user dismisses the dialog without selecting an account.
     *
     * @param dismissReason One of the DismissReason constants below.
     */
    protected final void onDismiss(int dismissReason) {
        if (mNativeController != 0) {
            FedCmAccountSelectionBridgeJni.get().onDismiss(mNativeController, dismissReason);
        }
    }

    /**
     * Call when the user taps "Use a different account" or "Continue" in the
     * failure dialog, triggering navigation to the IdP login page.
     */
    protected final void onLoginToIdP(String idpConfigUrl, String idpLoginUrl) {
        if (mNativeController != 0) {
            FedCmAccountSelectionBridgeJni.get().onLoginToIdP(
                    mNativeController, idpConfigUrl, idpLoginUrl);
        }
    }

    /** Call when the modal dialog opened by showModalDialog() has been closed. */
    protected final void onModalDialogClosed() {
        if (mNativeController != 0) {
            FedCmAccountSelectionBridgeJni.get().onModalDialogClosed(mNativeController);
        }
    }

    /**
     * Call when the user taps "More details" on the error dialog (shown only
     * when the IdP-provided {@link FedCmTokenError#getUrl()} is non-empty).
     *
     * <p>The native side then opens the error-details URL in a modal popup
     * via {@link #showModalDialog(String)}; do not open the URL from the UI
     * layer.
     */
    protected final void onMoreDetails() {
        if (mNativeController != 0) {
            FedCmAccountSelectionBridgeJni.get().onMoreDetails(mNativeController);
        }
    }

    // -------------------------------------------------------------------------
    // DismissReason constants — mirror content::IdentityRequestDialogController
    // -------------------------------------------------------------------------

    public static final int DISMISS_REASON_OTHER = 0;
    public static final int DISMISS_REASON_CLOSE_BUTTON = 1;
    public static final int DISMISS_REASON_SWIPE = 2;
    public static final int DISMISS_REASON_VIRTUAL_KEYBOARD = 3;
    public static final int DISMISS_REASON_GOT_IT_BUTTON = 4;
    public static final int DISMISS_REASON_MORE_DETAILS = 5;
    public static final int DISMISS_REASON_BACK_PRESS = 6;
    public static final int DISMISS_REASON_TAP_SCRIM = 7;

    // -------------------------------------------------------------------------
    // RpContext constants — mirror blink::mojom::RpContext
    // -------------------------------------------------------------------------

    public static final int RP_CONTEXT_SIGN_IN = 0;
    public static final int RP_CONTEXT_SIGN_UP = 1;
    public static final int RP_CONTEXT_USE = 2;
    public static final int RP_CONTEXT_CONTINUE = 3;

    // -------------------------------------------------------------------------
    // RpMode constants — mirror blink::mojom::RpMode
    // -------------------------------------------------------------------------

    public static final int RP_MODE_PASSIVE = 0;
    public static final int RP_MODE_ACTIVE = 1;


    public static final int DISCLOSURE_FIELD_NAME = 0;
    public static final int DISCLOSURE_FIELD_EMAIL = 1;
    public static final int DISCLOSURE_FIELD_PICTURE = 2;
    public static final int DISCLOSURE_FIELD_PHONE_NUMBER = 3;
    public static final int DISCLOSURE_FIELD_USERNAME = 4;

    // -------------------------------------------------------------------------
    // JNI declarations
    // -------------------------------------------------------------------------

    @NativeMethods
    interface Natives {
        void onAccountSelected(long nativeAlohaIdentityDialogController,
                String idpConfigUrl, String accountId, boolean isSignIn);
        void onDismiss(long nativeAlohaIdentityDialogController, int dismissReason);
        void onLoginToIdP(long nativeAlohaIdentityDialogController,
                String idpConfigUrl, String idpLoginUrl);
        void onModalDialogClosed(long nativeAlohaIdentityDialogController);
        void onMoreDetails(long nativeAlohaIdentityDialogController);
    }
}
