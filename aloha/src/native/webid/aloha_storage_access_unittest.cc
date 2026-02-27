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

#include "aloha/src/native/webid/aloha_storage_access.h"

#include "android_webview/browser/aw_cookie_access_policy.h"
#include "net/storage_access_api/status.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace aloha {

namespace {

using net::StorageAccessApiStatus;

const url::Origin kRp() {
  return url::Origin::Create(GURL("https://rp.example"));
}
const url::Origin kIdp() {
  return url::Origin::Create(GURL("https://accounts.idp.example"));
}
const GURL kIdpResource() {
  return GURL("https://accounts.idp.example/o/oauth2/iframe");
}

StorageAccessApiStatus Validate(const void* profile_key) {
  return ValidateStorageAccessStatus(StorageAccessApiStatus::kAccessViaAPI,
                                     profile_key, kRp(), kIdpResource());
}

TEST(AlohaStorageAccessTest, SpoofedStatusWithoutGrantIsRejected) {
  int profile_a = 0;
  EXPECT_EQ(Validate(&profile_a), StorageAccessApiStatus::kNone);
}

TEST(AlohaStorageAccessTest, GrantValidatesOnlyItsOwnProfile) {
  int profile_a = 0;
  int profile_b = 0;
  RecordStorageAccessGrantForTesting(&profile_a, kRp(), kIdp());
  EXPECT_EQ(Validate(&profile_a), StorageAccessApiStatus::kAccessViaAPI);
  EXPECT_EQ(Validate(&profile_b), StorageAccessApiStatus::kNone);
  ClearStorageAccessGrantsForProfile(&profile_a);
}

TEST(AlohaStorageAccessTest, kNoneStaysNoneEvenWithGrant) {
  int profile_a = 0;
  RecordStorageAccessGrantForTesting(&profile_a, kRp(), kIdp());
  EXPECT_EQ(ValidateStorageAccessStatus(StorageAccessApiStatus::kNone,
                                        &profile_a, kRp(), kIdpResource()),
            StorageAccessApiStatus::kNone);
  ClearStorageAccessGrantsForProfile(&profile_a);
}

// Regression test for profile-key reuse: a profile's grants must die with the
// profile, so a new profile allocated at the same address does not inherit
// them. The profile key is the profile's AwCookieAccessPolicy, which clears
// its grants in the destructor.
TEST(AlohaStorageAccessTest, GrantsDoNotSurviveProfileKeyReuse) {
  // Force deterministic address reuse: placement-construct two consecutive
  // policies in the same storage.
  alignas(android_webview::AwCookieAccessPolicy) unsigned char storage
      [sizeof(android_webview::AwCookieAccessPolicy)];

  auto* first = new (storage) android_webview::AwCookieAccessPolicy();
  RecordStorageAccessGrantForTesting(first, kRp(), kIdp());
  ASSERT_EQ(Validate(first), StorageAccessApiStatus::kAccessViaAPI);
  first->~AwCookieAccessPolicy();  // Profile deleted.

  // New profile at the reused address: must start with a clean slate.
  auto* second = new (storage) android_webview::AwCookieAccessPolicy();
  EXPECT_EQ(Validate(second), StorageAccessApiStatus::kNone);
  second->~AwCookieAccessPolicy();
}

}  // namespace

}  // namespace aloha
