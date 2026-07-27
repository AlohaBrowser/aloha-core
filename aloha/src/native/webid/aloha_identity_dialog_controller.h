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

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "third_party/jni_zero/jni_zero.h"
#include "base/memory/raw_ptr.h"
#include "content/public/browser/webid/identity_request_dialog_controller.h"

namespace content {
class WebContents;
}

namespace aloha {
// ALOHA: https://app.clickup.com/t/86ewz0pbr
// AlohaIdentityDialogController implements the FedCM dialog interface for
// Bromium (android_webview). It delegates the actual UI to a Java subclass of
// FedCmAccountSelectionBridge via JNI.
//
// Lifetime: owned by the content framework (unique_ptr returned from
// AwContentBrowserClient::CreateIdentityRequestDialogController).
// The Java bridge is created lazily on the first ShowXxxDialog call and is
// destroyed in the destructor.
class AlohaIdentityDialogController
    : public content::IdentityRequestDialogController {
 public:
  explicit AlohaIdentityDialogController(content::WebContents* web_contents);
  ~AlohaIdentityDialogController() override;

  // content::IdentityRequestDialogController:
  int GetBrandIconIdealSize(blink::mojom::RpMode rp_mode) override;
  int GetBrandIconMinimumSize(blink::mojom::RpMode rp_mode) override;

  bool ShowAccountsDialog(
      content::RelyingPartyData rp_data,
      const std::vector<scoped_refptr<content::IdentityProviderData>>& idp_list,
      const std::vector<scoped_refptr<content::IdentityRequestAccount>>& accounts,
      const std::vector<scoped_refptr<content::IdentityRequestAccount>>&
          filtered_accounts,
      blink::mojom::RpMode rp_mode,
      AccountSelectionCallback on_selected,
      LoginToIdPCallback on_add_account,
      DismissCallback dismiss_callback,
      AccountsDisplayedCallback accounts_displayed_callback) override;

  bool ShowFailureDialog(
      const content::RelyingPartyData& rp_data,
      const std::string& idp_for_display,
      blink::mojom::RpContext rp_context,
      blink::mojom::RpMode rp_mode,
      const content::IdentityProviderMetadata& idp_metadata,
      const std::vector<scoped_refptr<content::IdentityRequestAccount>>&
          filtered_accounts,
      DismissCallback dismiss_callback,
      LoginToIdPCallback login_callback) override;

  bool ShowErrorDialog(
      const content::RelyingPartyData& rp_data,
      const std::string& idp_for_display,
      blink::mojom::RpContext rp_context,
      blink::mojom::RpMode rp_mode,
      const content::IdentityProviderMetadata& idp_metadata,
      const std::optional<content::IdentityCredentialTokenError>& error,
      DismissCallback dismiss_callback,
      MoreDetailsCallback more_details_callback) override;

  bool ShowLoadingDialog(
      const content::RelyingPartyData& rp_data,
      const std::string& idp_for_display,
      blink::mojom::RpContext rp_context,
      blink::mojom::RpMode rp_mode,
      DismissCallback dismiss_callback) override;

  bool ShowVerifyingDialog(
      const content::RelyingPartyData& rp_data,
      const scoped_refptr<content::IdentityProviderData>& idp_data,
      const scoped_refptr<content::IdentityRequestAccount>& account,
      content::IdentityRequestAccount::SignInMode sign_in_mode,
      blink::mojom::RpMode rp_mode,
      AccountsDisplayedCallback accounts_displayed_callback) override;

  content::WebContents* ShowModalDialog(
      const GURL& url,
      blink::mojom::RpMode rp_mode,
      DismissCallback dismiss_callback,
      ShownModalAsyncCallback on_shown_async) override;

  void CloseModalDialog() override;
  content::WebContents* GetRpWebContents() override;

  // JNI callbacks — called from the DEFINE_JNI macro-generated entry points.
  // Signatures must match exactly what jni_zero generates from @NativeMethods.
  void OnAccountSelected(JNIEnv* env,
                         const jni_zero::JavaRef<jstring>& idp_config_url,
                         const jni_zero::JavaRef<jstring>& account_id,
                         bool is_sign_in);
  void OnDismiss(JNIEnv* env, int32_t dismiss_reason);
  void OnLoginToIdP(JNIEnv* env,
                    const jni_zero::JavaRef<jstring>& idp_config_url,
                    const jni_zero::JavaRef<jstring>& idp_login_url);
  void OnModalDialogClosed(JNIEnv* env);
  void OnMoreDetails(JNIEnv* env);

 private:
  // Creates the Java bridge object if not already created. Returns false if
  // the WebContents has no window (e.g., detached view).
  bool MaybeCreateBridge(blink::mojom::RpMode rp_mode);

  raw_ptr<content::WebContents> web_contents_;

  // Holds the FedCmAccountSelectionBridge Java object. Null until first show.
  base::android::ScopedJavaGlobalRef<jobject> java_bridge_;

  // Pending callbacks stored between ShowXxxDialog and the user action.
  AccountSelectionCallback on_account_selection_;
  LoginToIdPCallback on_login_;
  DismissCallback on_dismiss_;
  MoreDetailsCallback on_more_details_;
  AccountsDisplayedCallback on_accounts_displayed_;
};

}  // namespace aloha
