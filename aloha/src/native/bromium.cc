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

#include "bromium.h"

#include <iomanip>

#include "android_webview/browser_jni_headers/Bromium_jni.h"

#include "base/android/jni_string.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "android_webview/browser/aw_browser_context.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/storage_partition.h"
#include "components/download/public/common/download_task_runner.h"
#include "services/network/public/mojom/network_context.mojom.h"

#pragma GCC diagnostic ignored "-Wimplicit-const-int-float-conversion" // ALOHA https://app.clickup.com/t/86enzpzqc
// ALOHA https://app.clickup.com/t/861m81pn3
#include "third_party/blink/renderer/core/html/media/html_media_element.h"

// ALOHA https://app.clickup.com/t/86enm8q2x
#include "crypto/hmac.h"
#include "sign_source.h"
#include "android_webview/browser/aw_browser_context_store.h" 

#include "aloha_consts.h"
#include "log_assert_handler.h"

namespace aloha {
namespace {

// ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
// copy-paste from android_webview/browser/cookie_manager.cc
base::FilePath GetPathInAppDirectory(std::string path) {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_ANDROID_APP_DATA, &result)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }
  result = result.Append(FILE_PATH_LITERAL(path));
  return result;
}

// ALOHA https://app.clickup.com/t/2u59j0h?comment=1465391946
bool IsAllowedDownloadFromCacheImpl(const GURL& site_url) {
  constexpr std::string_view blacklist[] = {
    "instagram.com",
  };
  return std::end(blacklist) ==
    std::find_if(std::begin(blacklist), std::end(blacklist),
      [&] (const auto& block_domain) {
        return site_url.DomainIs({block_domain.data(), block_domain.size()});
      });
}

#if DCHECK_IS_ON()
void Test_IsAllowedDownloadFromCacheImpl() {
  static std::once_flag called;
  std::call_once(called,
    [] () {
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://www.instagram.com"}));
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://www.instagram.com/mkzartoosht6089/"}));
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://www.instaGram.com/mkzartoosht6089/"}));
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://instagram.com/mkzartoosht6089/"}));
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://cdn.instagram.com/mkzartoosht6089/"}));
      DCHECK(!IsAllowedDownloadFromCacheImpl(GURL{"https://cdn.n10.instagram.com/"}));
      DCHECK(IsAllowedDownloadFromCacheImpl(GURL{"https://www.google.com/"}));
      DCHECK(IsAllowedDownloadFromCacheImpl(GURL{"https://www.instagram.ru/mkzartoosht6089/"}));
      DCHECK(IsAllowedDownloadFromCacheImpl(GURL{"https://www.instagram.comi/mkzartoosht6089/"}));
      DCHECK(IsAllowedDownloadFromCacheImpl(GURL{"https://www.ainstagram.com/mkzartoosht6089/"}));
    });
}
#endif // #if DCHECK_IS_ON()

} // namespace

// ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
base::FilePath GetTemporaryDownloadsDir() {
  return GetPathInAppDirectory("bromium/TemporaryDownloads");
}

// ALOHA https://app.clickup.com/t/2u59j0h?comment=1465391946
bool IsAllowedDownloadFromCache(const GURL& site_url) {
#if DCHECK_IS_ON()
  Test_IsAllowedDownloadFromCacheImpl();
#endif

  return IsAllowedDownloadFromCacheImpl(site_url);
}

// ALOHA https://app.clickup.com/t/mz8wrn and https://app.clickup.com/t/2u59j0h
static void JNI_Bromium_ClearTemporaryDownloads(JNIEnv* env) {
  download::GetDownloadTaskRunner()->PostTask(
    FROM_HERE,
    base::BindOnce([] () {
      auto dir = GetTemporaryDownloadsDir();
      if (!base::DeletePathRecursively(dir)) {
        LOG(ERROR) << "Failed to remove directory '" << dir << "'.";
        return;
      }
      if (!base::CreateDirectory(dir)) {
        LOG(ERROR) << "Failed to create directory '" << dir << "'.";
      }
    }));
}

static void JNI_Bromium_SetJavaCallstackFilename(JNIEnv* env, const
    base::android::JavaParamRef<jstring>& file_name) {
        strncpy(aloha::g_callstack_file_name, base::android::ConvertJavaStringToUTF8(env, file_name).c_str(), sizeof(g_callstack_file_name));
}

// ALOHA https://app.clickup.com/t/2f29z75
static void JNI_Bromium_SetSendDNTHeader(JNIEnv* env, jboolean send) {
    auto* net_ctx = android_webview::AwBrowserContext::GetDefault()
                      ->GetDefaultStoragePartition()
                      ->GetNetworkContext();
    if (net_ctx) {
        net_ctx->SetSendDNTHeader(send);
    }
}

// ALOHA https://app.clickup.com/t/2hcppgv
static void JNI_Bromium_ClearSessionStorage(JNIEnv* env) {
  android_webview::AwBrowserContext::GetDefault()->ForEachLoadedStoragePartition(
     [](content::StoragePartition* storage) {
        storage->ClearSessionStorage();
      });
}

// ALOHA https://app.clickup.com/t/2hcppgv
static void JNI_Bromium_ClearLocalStorage(JNIEnv* env, jboolean for_private_mode) {
  auto* context = android_webview::AwBrowserContext::GetDefault();
  DCHECK(context != nullptr);
  context->ForEachLoadedStoragePartition(
      [&](content::StoragePartition* storage) {
        storage->ClearLocalStorage(for_private_mode);
      });

  // https://app.clickup.com/t/2hjmvup?comment=90040001732069
  if (!for_private_mode) {
    // Local storage do not save for private mode.
    context->GetBrowsingDataRemover()->Remove(
        base::Time(), base::Time::Max(),
        content::BrowsingDataRemover::DATA_TYPE_LOCAL_STORAGE,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
            content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB);
  }
}

// ALOHA https://app.clickup.com/t/2tzcywg
static void JNI_Bromium_AddIgnoredDcheck(JNIEnv* env, const base::android::JavaParamRef<jstring>& ignore_substring) {
  AddIgnoredDcheck(base::android::ConvertJavaStringToUTF8(env, ignore_substring));
}

// ALOHA https://app.clickup.com/t/86enm8q2x
static base::android::ScopedJavaLocalRef<jstring> JNI_Bromium_GetSignature(JNIEnv* env,  const
    base::android::JavaParamRef<jstring>& message) {
    
    [[maybe_unused]] static volatile auto* context_store = android_webview::AwBrowserContextStore::GetInstance(); // ALOHA Usage check only in application
    if(!context_store) // ALOHA This check is for disabling compiler optimization
    {
      LOG(ERROR) << "AwBrowserContextStore is not initialized";
      return base::android::ScopedJavaLocalRef<jstring>();
    }

    const int kDidgestLength{32};
    static crypto::HMAC hmac(crypto::HMAC::HashAlgorithm::SHA256);
    static const std::string key = GenrateSignSource();
    std::array<unsigned char, kDidgestLength> didgets;
    
    if( hmac.Init(key) && hmac.Sign(base::android::ConvertJavaStringToUTF8(env, message), didgets.data(), kDidgestLength))
    {
      std::stringstream output_stream;
      output_stream << std::setfill('0') << std::hex;    
      for(int i=0; i < kDidgestLength; i++){
        output_stream << std::setw(2) << static_cast<unsigned>(didgets[i]);
      }

      return base::android::ConvertUTF8ToJavaString(env, output_stream.str());
    }    
    return base::android::ScopedJavaLocalRef<jstring>();
  }

} // namespace aloha
