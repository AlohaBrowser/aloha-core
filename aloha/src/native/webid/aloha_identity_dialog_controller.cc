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

#include "aloha_identity_dialog_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/webid/identity_request_account.h"
#include "content/public/browser/webid/identity_request_dialog_controller.h"
#include "third_party/blink/public/mojom/webid/federated_auth_request.mojom.h"
#include "ui/android/color_utils_android.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/gfx/android/java_bitmap.h"
#include "url/gurl.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/browser_jni_headers/FedCmAccountSelectionBridge_jni.h"
#include "android_webview/browser_jni_headers/FedCmAccount_jni.h"
#include "android_webview/browser_jni_headers/FedCmClientIdMetadata_jni.h"
#include "android_webview/browser_jni_headers/FedCmIdentityProviderData_jni.h"
#include "android_webview/browser_jni_headers/FedCmIdentityProviderMetadata_jni.h"
#include "android_webview/browser_jni_headers/FedCmRelyingPartyData_jni.h"
#include "android_webview/browser_jni_headers/FedCmTokenError_jni.h"
#include "third_party/jni_zero/jni_zero.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaIntArray;
using content::IdentityProviderData;
using content::IdentityProviderMetadata;
using content::IdentityRequestAccount;
using content::IdentityRequestDialogController;
using DismissReason = content::IdentityRequestDialogController::DismissReason;

namespace {
// The RP's WebContents active during ShowModalDialog. Used by GetRpWebContents()
// when called from the popup's fresh controller (no java_bridge_).
// There is at most one active FedCM modal dialog at a time.
//
// Lifetime invariant: cleared by the owning AlohaIdentityDialogController's
// destructor (the controller that called ShowModalDialog), which runs when
// the RP's WebContents is torn down. By the time static destruction runs the
// pointer is therefore already null, so raw_ptr<> needs no dangling traits.
// /* ALOHA: Android modal fallback */
raw_ptr<content::WebContents> g_modal_rp_web_contents = nullptr;
}  // namespace

namespace aloha {

namespace {

// ---------------------------------------------------------------------------
// Serialisation helpers (C++ structs → Java data objects)
// ---------------------------------------------------------------------------

ScopedJavaLocalRef<jintArray> ConvertFieldsToJavaArray(
    JNIEnv* env,
    const std::vector<content::IdentityRequestDialogDisclosureField>& fields) {
  std::vector<int> ints;
  ints.reserve(fields.size());
  for (auto f : fields) {
    ints.push_back(static_cast<int>(f));
  }
  return ToJavaIntArray(env, ints);
}

ScopedJavaLocalRef<jobject> ConvertToJavaIdentityProviderMetadata(
    JNIEnv* env,
    const IdentityProviderMetadata& meta,
    blink::mojom::RpMode rp_mode) {
  ScopedJavaLocalRef<jobject> icon;
  if (!meta.brand_decoded_icon.IsEmpty()) {
    icon = gfx::ConvertToJavaBitmap(*meta.brand_decoded_icon.ToSkBitmap());
  }
  bool show_different =
      rp_mode == blink::mojom::RpMode::kPassive
          ? meta.has_filtered_out_account
          : meta.supports_add_account || meta.has_filtered_out_account;
  return Java_FedCmIdentityProviderMetadata_Constructor(
      env,
      ui::OptionalSkColorToJavaColor(meta.brand_text_color),
      ui::OptionalSkColorToJavaColor(meta.brand_background_color),
      icon,
      ConvertUTF8ToJavaString(env, meta.config_url.spec()),
      ConvertUTF8ToJavaString(env, meta.idp_login_url.spec()),
      show_different);
}

ScopedJavaLocalRef<jobject> ConvertToJavaClientIdMetadata(
    JNIEnv* env,
    const content::ClientMetadata& meta) {
  ScopedJavaLocalRef<jobject> icon;
  if (!meta.brand_decoded_icon.IsEmpty()) {
    icon = gfx::ConvertToJavaBitmap(*meta.brand_decoded_icon.ToSkBitmap());
  }
  return Java_FedCmClientIdMetadata_Constructor(
      env,
      ConvertUTF8ToJavaString(env, meta.terms_of_service_url.spec()),
      ConvertUTF8ToJavaString(env, meta.privacy_policy_url.spec()),
      icon);
}

ScopedJavaLocalRef<jobject> ConvertToJavaIdentityProviderData(
    JNIEnv* env,
    const IdentityProviderData& idp,
    blink::mojom::RpMode rp_mode) {
  return Java_FedCmIdentityProviderData_Constructor(
      env,
      ConvertUTF8ToJavaString(env, idp.idp_for_display),
      ConvertToJavaIdentityProviderMetadata(env, idp.idp_metadata, rp_mode),
      ConvertToJavaClientIdMetadata(env, idp.client_metadata),
      static_cast<int32_t>(idp.rp_context),
      ConvertFieldsToJavaArray(env, idp.disclosure_fields),
      idp.has_login_status_mismatch);
}

ScopedJavaLocalRef<jobject> ConvertToJavaRelyingPartyData(
    JNIEnv* env,
    const content::RelyingPartyData& rp) {
  ScopedJavaLocalRef<jobject> icon;
  if (!rp.rp_icon.IsEmpty()) {
    icon = gfx::ConvertToJavaBitmap(*rp.rp_icon.ToSkBitmap());
  }
  return Java_FedCmRelyingPartyData_Constructor(
      env,
      ConvertUTF16ToJavaString(env, rp.rp_for_display),
      ConvertUTF16ToJavaString(env, rp.iframe_for_display),
      icon,
      rp.display_strings_may_change);
}

ScopedJavaLocalRef<jobject> ConvertToJavaTokenError(
    JNIEnv* env,
    const std::optional<content::IdentityCredentialTokenError>& error) {
  return Java_FedCmTokenError_Constructor(
      env,
      ConvertUTF8ToJavaString(env, error ? error->code : std::string()),
      ConvertUTF8ToJavaString(env, error ? error->url.spec() : std::string()));
}

// Build a jobjectArray of FedCmAccount objects. |idp_map| maps each
// IdentityProviderData scoped_refptr to its already-constructed Java peer so
// we don't re-create the Java object for every account.
ScopedJavaLocalRef<jobjectArray> ConvertToJavaAccounts(
    JNIEnv* env,
    const std::vector<scoped_refptr<IdentityRequestAccount>>& accounts,
    const base::flat_map<scoped_refptr<IdentityProviderData>,
                         ScopedJavaLocalRef<jobject>>& idp_map,
    bool is_multi_idp,
    float device_scale_factor) {
  static const char kAccountClass[] =
      "com/alohamobile/bromium/webid/FedCmAccount";
  ScopedJavaLocalRef<jclass> clazz =
      base::android::GetClass(env, kAccountClass);
  auto array = ScopedJavaLocalRef<jobjectArray>::Adopt(
      env,
      env->NewObjectArray(static_cast<jsize>(accounts.size()),
                          clazz.obj(), nullptr));
  base::android::CheckException(env);

  for (size_t i = 0; i < accounts.size(); ++i) {
    const IdentityRequestAccount& acc = *accounts[i];

    ScopedJavaLocalRef<jobject> avatar;
    if (!acc.decoded_picture.IsEmpty()) {
      avatar = gfx::ConvertToJavaBitmap(*acc.decoded_picture.ToSkBitmap());
    }

    // multi-IDP: circle-crop with IDP badge (skip for Stage 1 — pass null)
    ScopedJavaLocalRef<jobject> badged_avatar;

    std::string secondary_desc;
    if (is_multi_idp) {
      secondary_desc = acc.identity_provider->idp_for_display;
    }

    ScopedJavaLocalRef<jobject> java_idp =
        idp_map.at(acc.identity_provider);

    ScopedJavaLocalRef<jobject> item = Java_FedCmAccount_Constructor(
        env,
        ConvertUTF8ToJavaString(env, acc.id),
        ConvertUTF8ToJavaString(env, acc.display_identifier),
        ConvertUTF8ToJavaString(env, acc.display_name),
        ConvertUTF8ToJavaString(env, acc.given_name),
        ConvertUTF8ToJavaString(env, secondary_desc),
        avatar,
        badged_avatar,
        acc.idp_claimed_login_state == IdentityRequestAccount::LoginState::kSignIn,
        acc.browser_trusted_login_state == IdentityRequestAccount::LoginState::kSignIn,
        acc.is_filtered_out,
        ConvertFieldsToJavaArray(env, acc.fields),
        java_idp);

    env->SetObjectArrayElement(array.obj(), static_cast<jsize>(i), item.obj());
  }
  return array;
}

ScopedJavaLocalRef<jobjectArray> ConvertToJavaIdpArray(
    JNIEnv* env,
    const base::flat_map<scoped_refptr<IdentityProviderData>,
                         ScopedJavaLocalRef<jobject>>& idp_map) {
  static const char kIdpClass[] =
      "com/alohamobile/bromium/webid/FedCmIdentityProviderData";
  ScopedJavaLocalRef<jclass> clazz =
      base::android::GetClass(env, kIdpClass);
  auto array = ScopedJavaLocalRef<jobjectArray>::Adopt(
      env,
      env->NewObjectArray(static_cast<jsize>(idp_map.size()),
                          clazz.obj(), nullptr));
  base::android::CheckException(env);
  jsize idx = 0;
  for (const auto& [key, val] : idp_map) {
    env->SetObjectArrayElement(array.obj(), idx++, val.obj());
  }
  return array;
}

}  // namespace

// ---------------------------------------------------------------------------
// AlohaIdentityDialogController
// ---------------------------------------------------------------------------

AlohaIdentityDialogController::AlohaIdentityDialogController(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

AlohaIdentityDialogController::~AlohaIdentityDialogController() {
  if (!java_bridge_.is_null()) {
    Java_FedCmAccountSelectionBridge_destroy(AttachCurrentThread(),
                                             java_bridge_);
  }
  if (g_modal_rp_web_contents == web_contents_) {
    g_modal_rp_web_contents = nullptr; /* ALOHA: Android modal fallback */
  }
}

bool AlohaIdentityDialogController::MaybeCreateBridge(
    blink::mojom::RpMode rp_mode) {
  if (!java_bridge_.is_null()) {
    return true;
  }

  auto* native_view = web_contents_->GetNativeView();
  if (!native_view) {
    return false;
  }
  auto* window = native_view->GetWindowAndroid();
  if (!window) {
    return false;
  }

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> bridge =
      Java_FedCmAccountSelectionBridge_createFromNative(
          env,
          reinterpret_cast<jlong>(this),
          web_contents_->GetJavaWebContents(),
          window->GetJavaObject(),
          static_cast<int32_t>(rp_mode));

  if (bridge.is_null()) {
    return false;
  }
  java_bridge_ = base::android::ScopedJavaGlobalRef<jobject>(bridge);
  return true;
}

int AlohaIdentityDialogController::GetBrandIconIdealSize(
    blink::mojom::RpMode /*rp_mode*/) {
  return 20;
}

int AlohaIdentityDialogController::GetBrandIconMinimumSize(
    blink::mojom::RpMode /*rp_mode*/) {
  return 16;
}

bool AlohaIdentityDialogController::ShowAccountsDialog(
    content::RelyingPartyData rp_data,
    const std::vector<scoped_refptr<IdentityProviderData>>& idp_list,
    const std::vector<scoped_refptr<IdentityRequestAccount>>& accounts,
    const std::vector<scoped_refptr<IdentityRequestAccount>>& filtered_accounts,
    blink::mojom::RpMode rp_mode,
    AccountSelectionCallback on_selected,
    LoginToIdPCallback on_add_account,
    DismissCallback dismiss_callback,
    AccountsDisplayedCallback accounts_displayed_callback) {
  if (!MaybeCreateBridge(rp_mode)) {
    std::move(dismiss_callback).Run(DismissReason::kOther);
    return false;
  }

  on_account_selection_ = std::move(on_selected);
  on_login_ = std::move(on_add_account);
  on_dismiss_ = std::move(dismiss_callback);
  on_accounts_displayed_ = std::move(accounts_displayed_callback);

  JNIEnv* env = AttachCurrentThread();

  // Build IDP map: C++ ptr → Java object (keyed by scoped_refptr value)
  base::flat_map<scoped_refptr<IdentityProviderData>,
                 ScopedJavaLocalRef<jobject>> idp_map;
  for (const auto& idp : idp_list) {
    idp_map[idp] = ConvertToJavaIdentityProviderData(env, *idp, rp_mode);
  }

  bool is_multi_idp = idp_list.size() > 1u;
  float scale = web_contents_->GetPrimaryMainFrame()
                    ->GetRenderWidgetHost()
                    ->GetDeviceScaleFactor();

  ScopedJavaLocalRef<jobjectArray> j_accounts =
      ConvertToJavaAccounts(env, accounts, idp_map, is_multi_idp, scale);

  // new_accounts: accounts with DisplayPriority::kNew (just signed in).
  // For now pass an empty array — the engine handles highlighting separately.
  std::vector<scoped_refptr<IdentityRequestAccount>> new_accounts;
  ScopedJavaLocalRef<jobjectArray> j_new_accounts =
      ConvertToJavaAccounts(env, new_accounts, idp_map, is_multi_idp, scale);

  ScopedJavaLocalRef<jobjectArray> j_idps =
      ConvertToJavaIdpArray(env, idp_map);

  bool shown = Java_FedCmAccountSelectionBridge_showAccounts(
      env, java_bridge_,
      ConvertToJavaRelyingPartyData(env, rp_data),
      j_accounts, j_idps, j_new_accounts);

  if (!shown) {
    // Java told us it couldn't show UI — clean up and report dismiss.
    on_account_selection_.Reset();
    on_login_.Reset();
    on_accounts_displayed_.Reset();
    std::move(on_dismiss_).Run(DismissReason::kOther);
    return false;
  }

  if (on_accounts_displayed_) {
    std::move(on_accounts_displayed_).Run();
  }
  return true;
}

bool AlohaIdentityDialogController::ShowFailureDialog(
    const content::RelyingPartyData& rp_data,
    const std::string& idp_for_display,
    blink::mojom::RpContext rp_context,
    blink::mojom::RpMode rp_mode,
    const IdentityProviderMetadata& idp_metadata,
    const std::vector<scoped_refptr<IdentityRequestAccount>>& filtered_accounts,
    DismissCallback dismiss_callback,
    LoginToIdPCallback login_callback) {
  if (!MaybeCreateBridge(rp_mode)) {
    std::move(dismiss_callback).Run(DismissReason::kOther);
    return false;
  }

  on_dismiss_ = std::move(dismiss_callback);
  on_login_ = std::move(login_callback);

  JNIEnv* env = AttachCurrentThread();
  bool shown = Java_FedCmAccountSelectionBridge_showFailureDialog(
      env, java_bridge_,
      ConvertToJavaRelyingPartyData(env, rp_data),
      ConvertUTF8ToJavaString(env, idp_for_display),
      ConvertToJavaIdentityProviderMetadata(env, idp_metadata, rp_mode),
      static_cast<int32_t>(rp_context));

  if (!shown) {
    on_login_.Reset();
    std::move(on_dismiss_).Run(DismissReason::kOther);
    return false;
  }
  return true;
}

bool AlohaIdentityDialogController::ShowErrorDialog(
    const content::RelyingPartyData& rp_data,
    const std::string& idp_for_display,
    blink::mojom::RpContext rp_context,
    blink::mojom::RpMode rp_mode,
    const IdentityProviderMetadata& idp_metadata,
    const std::optional<content::IdentityCredentialTokenError>& error,
    DismissCallback dismiss_callback,
    MoreDetailsCallback more_details_callback) {
  if (!MaybeCreateBridge(rp_mode)) {
    std::move(dismiss_callback).Run(DismissReason::kOther);
    return false;
  }

  on_dismiss_ = std::move(dismiss_callback);
  on_more_details_ = std::move(more_details_callback);

  JNIEnv* env = AttachCurrentThread();
  bool shown = Java_FedCmAccountSelectionBridge_showErrorDialog(
      env, java_bridge_,
      ConvertToJavaRelyingPartyData(env, rp_data),
      ConvertUTF8ToJavaString(env, idp_for_display),
      ConvertToJavaIdentityProviderMetadata(env, idp_metadata, rp_mode),
      static_cast<int32_t>(rp_context),
      ConvertToJavaTokenError(env, error));

  if (!shown) {
    on_more_details_.Reset();
    std::move(on_dismiss_).Run(DismissReason::kOther);
    return false;
  }
  return true;
}

bool AlohaIdentityDialogController::ShowLoadingDialog(
    const content::RelyingPartyData& rp_data,
    const std::string& idp_for_display,
    blink::mojom::RpContext rp_context,
    blink::mojom::RpMode rp_mode,
    DismissCallback dismiss_callback) {
  if (!MaybeCreateBridge(rp_mode)) {
    std::move(dismiss_callback).Run(DismissReason::kOther);
    return false;
  }

  on_dismiss_ = std::move(dismiss_callback);

  JNIEnv* env = AttachCurrentThread();
  bool shown = Java_FedCmAccountSelectionBridge_showLoadingDialog(
      env, java_bridge_,
      ConvertToJavaRelyingPartyData(env, rp_data),
      ConvertUTF8ToJavaString(env, idp_for_display),
      static_cast<int32_t>(rp_context));

  if (!shown) {
    std::move(on_dismiss_).Run(DismissReason::kOther);
    return false;
  }
  return true;
}

bool AlohaIdentityDialogController::ShowVerifyingDialog(
    const content::RelyingPartyData& rp_data,
    const scoped_refptr<IdentityProviderData>& idp_data,
    const scoped_refptr<IdentityRequestAccount>& account,
    IdentityRequestAccount::SignInMode sign_in_mode,
    blink::mojom::RpMode rp_mode,
    AccountsDisplayedCallback accounts_displayed_callback) {
  if (!MaybeCreateBridge(rp_mode)) {
    return false;
  }

  on_accounts_displayed_ = std::move(accounts_displayed_callback);

  JNIEnv* env = AttachCurrentThread();

  base::flat_map<scoped_refptr<IdentityProviderData>,
                 ScopedJavaLocalRef<jobject>> idp_map;
  idp_map[idp_data] =
      ConvertToJavaIdentityProviderData(env, *idp_data, rp_mode);

  float scale = web_contents_->GetPrimaryMainFrame()
                    ->GetRenderWidgetHost()
                    ->GetDeviceScaleFactor();

  ScopedJavaLocalRef<jobjectArray> tmp =
      ConvertToJavaAccounts(env, {account}, idp_map, /*is_multi_idp=*/false,
                            scale);
  ScopedJavaLocalRef<jobject> j_account =
      ScopedJavaLocalRef<jobject>::Adopt(env, env->GetObjectArrayElement(tmp.obj(), 0));

  bool shown = Java_FedCmAccountSelectionBridge_showVerifyingDialog(
      env, java_bridge_,
      ConvertToJavaRelyingPartyData(env, rp_data),
      j_account,
      sign_in_mode == IdentityRequestAccount::SignInMode::kAuto);

  if (shown && on_accounts_displayed_) {
    std::move(on_accounts_displayed_).Run();
  }
  return shown;
}

content::WebContents* AlohaIdentityDialogController::ShowModalDialog(
    const GURL& url,
    blink::mojom::RpMode rp_mode,
    DismissCallback dismiss_callback,
    ShownModalAsyncCallback on_shown_async) {
  // ALOHA: Android returns the modal WebContents synchronously below, so the
  // deferred on_shown_async path (used when ShowModalDialog returns null and
  // the WebContents arrives later) is intentionally unused here.
  if (!MaybeCreateBridge(rp_mode)) {
    std::move(dismiss_callback).Run(DismissReason::kOther);
    return nullptr;
  }
  on_dismiss_ = std::move(dismiss_callback);
  // Cache the RP WebContents so GetRpWebContents() can serve popup controllers
  // that have no java_bridge_ (Android: ShowModalDialog returns null).
  g_modal_rp_web_contents = web_contents_; /* ALOHA: Android modal fallback */

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_wc =
      Java_FedCmAccountSelectionBridge_showModalDialog(
          env, java_bridge_, ConvertUTF8ToJavaString(env, url.spec()));
  if (j_wc.is_null()) {
    return nullptr;
  }
  return content::WebContents::FromJavaWebContents(j_wc);
}

void AlohaIdentityDialogController::CloseModalDialog() {
  if (java_bridge_.is_null()) {
    return;
  }
  // Engine-initiated close — drop the pending dismiss callback before the
  // asynchronous Java Activity teardown runs. Without this, when OnClose()'s
  // Android fallback path calls CloseModalDialog() and then starts a re-fetch,
  // the Activity's eventual onDestroy → onModalDialogClosed JNI call would
  // fire on_dismiss_ (= RequestService::OnDialogDismissed) and abort the
  // in-flight re-fetch with kUiDismissedNoEmbargo. /* ALOHA: FedCM */
  on_dismiss_.Reset();
  Java_FedCmAccountSelectionBridge_closeModalDialog(AttachCurrentThread(),
                                                     java_bridge_);
}

content::WebContents* AlohaIdentityDialogController::GetRpWebContents() {
  if (java_bridge_.is_null()) {
    // Called from a popup context: no bridge, return cached RP WebContents.
    // SetupIdentityRegistryFromPopup() uses this to link IdentityProvider.close()
    // in the popup back to the RP's pending FedCM request. /* ALOHA: Android modal fallback */
    return g_modal_rp_web_contents;
  }
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_wc =
      Java_FedCmAccountSelectionBridge_getRpWebContents(env, java_bridge_);
  if (j_wc.is_null()) {
    return nullptr;
  }
  return content::WebContents::FromJavaWebContents(j_wc);
}

// ---------------------------------------------------------------------------
// JNI callbacks — invoked from Java
// ---------------------------------------------------------------------------

void AlohaIdentityDialogController::OnAccountSelected(
    JNIEnv* env,
    const jni_zero::JavaRef<jstring>& idp_config_url,
    const jni_zero::JavaRef<jstring>& account_id,
    bool is_sign_in) {
  if (on_account_selection_) {
    std::move(on_account_selection_)
        .Run(GURL(ConvertJavaStringToUTF8(env, idp_config_url)),
             ConvertJavaStringToUTF8(env, account_id),
             is_sign_in);
  }
}

void AlohaIdentityDialogController::OnDismiss(JNIEnv* /*env*/,
                                               int32_t dismiss_reason) {
  if (on_dismiss_) {
    std::move(on_dismiss_).Run(static_cast<DismissReason>(dismiss_reason));
  }
}

void AlohaIdentityDialogController::OnLoginToIdP(
    JNIEnv* env,
    const jni_zero::JavaRef<jstring>& idp_config_url,
    const jni_zero::JavaRef<jstring>& idp_login_url) {
  if (on_login_) {
    on_login_.Run(GURL(ConvertJavaStringToUTF8(env, idp_config_url)),
                  GURL(ConvertJavaStringToUTF8(env, idp_login_url)));
  }
}

void AlohaIdentityDialogController::OnModalDialogClosed(JNIEnv* /*env*/) {
  if (on_dismiss_) {
    std::move(on_dismiss_).Run(DismissReason::kOther);
  }
}

void AlohaIdentityDialogController::OnMoreDetails(JNIEnv* /*env*/) {
  if (on_more_details_) {
    std::move(on_more_details_).Run();
  }
}

// ---------------------------------------------------------------------------
// JNI static registration (jni_zero style)
// DEFINE_JNI generates the export functions that Java calls into.
// ---------------------------------------------------------------------------

DEFINE_JNI(FedCmAccountSelectionBridge)
DEFINE_JNI(FedCmAccount)
DEFINE_JNI(FedCmClientIdMetadata)
DEFINE_JNI(FedCmIdentityProviderData)
DEFINE_JNI(FedCmIdentityProviderMetadata)
DEFINE_JNI(FedCmRelyingPartyData)
DEFINE_JNI(FedCmTokenError)

}  // namespace aloha
