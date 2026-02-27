// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_browser_context_store.h"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_browser_process.h"
#include "android_webview/browser/cookie_manager.h"
#include "android_webview/common/aw_features.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/metrics/histogram_functions.h"
#include "base/functional/function_ref.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
// ALOHA https://app.clickup.com/t/2558578/86eyfccqg
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/spare_render_process_host_manager.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/browser_jni_headers/AwBrowserContextStore_jni.h"

namespace android_webview {

namespace {

constexpr char kProfileNameKey[] = "name";
constexpr char kProfilePathKey[] = "path";

// ALOHA: read from non-UI threads via IsInitialized(), so make it atomic.
std::atomic<bool> g_initialized = false;

// ALOHA CU-86eqe7rd2 (U2): migration enabled — the ALOHA cookie mechanism is
// removed (U5+U7), the old code no longer recreates bromium/Cookies.*, so the
// one-shot move to the profile paths is safe.
constexpr bool kMigrateBromiumModeCookies = true;

// ALOHA: SQLite sidecar file names (rollback journal; WebView does not use WAL —
// if upstream switches to WAL, add -wal/-shm).
base::FilePath JournalPath(const base::FilePath& db) {
  return base::FilePath(db.value() + FILE_PATH_LITERAL("-journal"));
}

// ALOHA: staging name for the copy-then-rename publish in
// bromium_migration::MigrateCookieStorePair.
base::FilePath TempCopyPath(const base::FilePath& file) {
  return base::FilePath(file.value() + FILE_PATH_LITERAL(".migrating"));
}

}  // namespace

// ALOHA CU-86eqe7rd2 (U2): see the header.
namespace bromium_migration {

bool MigrateCookieStorePair(const base::FilePath& src_db,
                            const base::FilePath& dest_db) {
  if (!base::PathExists(src_db)) {
    // Idempotency: no source db — the mode was never used.
    return true;
  }
  const base::FilePath src_journal = JournalPath(src_db);
  const base::FilePath dest_journal = JournalPath(dest_db);
  const base::FilePath tmp_db = TempCopyPath(dest_db);
  const base::FilePath tmp_journal = TempCopyPath(dest_journal);
  const bool has_src_journal = base::PathExists(src_journal);

  // Stage into temp files next to dest first: the common failure (ENOSPC,
  // I/O error) must leave BOTH the source and the existing dest untouched, so
  // the session keeps running on whatever store it had and the next start
  // retries from unchanged inputs. Only cheap same-directory renames follow.
  if (!base::CopyFile(src_db, tmp_db) ||
      (has_src_journal && !base::CopyFile(src_journal, tmp_journal))) {

    base::DeleteFile(tmp_db);
    base::DeleteFile(tmp_journal);
    return false;
  }

  // Publish. Ordering is load-bearing:
  // 1. Drop the dest journal first — SQLite pairs a rollback journal with its
  //    db by file name alone, so a db must never coexist with a foreign
  //    (pre-profile builds') hot journal: it would be replayed into it,
  //    corrupting it. With the migration marker absent an existing dest pair
  //    is such a stale artifact (the old builds kept an empty store at
  //    Default/Cookies), not user data.
  // 2. Then the journal, then the db. The db rename is the commit point and
  //    must come LAST: the caller treats `false` as "dest untouched, retry
  //    next start", so the db must never be published on a path that can
  //    still fail — otherwise a migrated db would go live without its hot
  //    journal (losing SQLite's torn-transaction rollback) and the next
  //    start's retry would overwrite a store the session has already opened.
  //    A crash between the two renames leaves the old (stale, pre-marker) db
  //    next to the new journal, but the next start re-runs the migration and
  //    step 1 deletes that journal again before anything opens the store.
  if (!base::DeleteFile(dest_journal)) {
    base::DeleteFile(tmp_db);
    base::DeleteFile(tmp_journal);
    return false;
  }
  if (has_src_journal &&
      !base::ReplaceFile(tmp_journal, dest_journal, nullptr)) {
    base::DeleteFile(tmp_db);
    base::DeleteFile(tmp_journal);
    return false;
  }
  if (!base::ReplaceFile(tmp_db, dest_db, nullptr)) {
    // Never leave the just-published journal paired with the old db — SQLite
    // would replay it into it on open. The old db predates the marker and is
    // a stale artifact, so dropping its (already deleted) journal is fine.
    base::DeleteFile(dest_journal);
    base::DeleteFile(tmp_db);
    base::DeleteFile(tmp_journal);
    return false;
  }
  return true;
}

void DeleteCookieStorePair(const base::FilePath& db) {
  base::DeleteFile(db);
  base::DeleteFile(JournalPath(db));
}

void MoveBromiumNetworkStateFiles(const base::FilePath& bromium_dir,
                                  const base::FilePath& default_dir) {
  base::FileEnumerator files(bromium_dir, /*recursive=*/false,
                             base::FileEnumerator::FILES);
  for (base::FilePath src = files.Next(); !src.empty(); src = files.Next()) {
    const base::FilePath::StringType name = src.BaseName().value();
    if (name.rfind(FILE_PATH_LITERAL("Cookies."), 0) == 0) {
      // Cookies.<inst> and their journals are migrated by MigrateCookieStorePair.
      continue;
    }
    const base::FilePath dest = default_dir.Append(src.BaseName());
    if (base::PathExists(dest)) {
      // Never overwrite live network state (same rationale as
      // MigrateCookieStorePair) — drop the stale source instead.
      base::DeleteFile(src);
      continue;
    }
    if (!base::Move(src, dest)) {
      LOG(ERROR) << "Bromium cookie migration: failed to move network state "
                 << src.value();
    }
  }
}

}  // namespace bromium_migration

AwBrowserContextStore::AwBrowserContextStore(PrefService* pref_service)
    : prefs_(*pref_service) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  TRACE_EVENT0("startup", "AwBrowserContextStore::AwBrowserContextStore");

  // The pref store tracks the profile and directory names of all non-default
  // profiles. The default profile exists implicitly and is not tracked in the
  // pref store.
  ScopedListPrefUpdate update(&*prefs_, prefs::kProfileListPref);
  base::ListValue& profiles = update.Get();
  for (const auto& profile : profiles) {
    const base::DictValue& profile_dict = profile.GetDict();
    const std::string* name = profile_dict.FindString(kProfileNameKey);
    CHECK(name);
    const std::string* path_string = profile_dict.FindString(kProfilePathKey);
    CHECK(path_string);
    CHECK(!path_string->empty());
    CHECK_NE(*path_string, base::FilePath::kCurrentDirectory);
    CHECK_NE(*path_string, base::FilePath::kParentDirectory);
    CHECK_EQ(path_string->find_first_of(base::FilePath::kSeparators),
             std::string::npos);
    base::FilePath path = base::FilePath(*path_string);
    const bool name_is_new =
        contexts_.emplace(std::string(*name), Entry(std::move(path), nullptr))
            .second;
    CHECK(name_is_new);
  }
  base::UmaHistogramCounts100(
      "Android.WebView.AwBrowserContext.NonDefault.CountAtStartup",
      profiles.size());

  // ALOHA CU-86eqe7rd2 (U2): one-shot migration of the ALOHA-mode cookie files.
  // Strictly here: contexts are not yet instantiated, the cookie store is not
  // open, g_initialized is still false.
  if (kMigrateBromiumModeCookies) {
    MigrateBromiumModeCookies();
  }

  // ALOHA https://app.clickup.com/t/2558578/86eyfccqg: let the eyeo SubscriptionServiceFactory tell the
  // default profile apart from non-default ones. Non-default profiles never
  // download filter lists; they reuse the Default profile's downloaded
  // subscriptions via SharedSubscriptionServiceDecorator. Registered here,
  // before any context (and thus any adblock KeyedService) is instantiated.
  adblock::SubscriptionServiceFactory::SetDefaultBrowserContextGetter(
      base::BindRepeating([]() -> content::BrowserContext* {
        if (!AwBrowserContextStore::IsInitialized()) {
          return nullptr;
        }
        return AwBrowserContextStore::GetInstance()->GetDefault();
      }));
}

// ALOHA CU-86eqe7rd2 (U2): see the header.
void AwBrowserContextStore::MigrateBromiumModeCookies() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  TRACE_EVENT0("startup", "AwBrowserContextStore::MigrateBromiumModeCookies");
  // Migration window: before any context is instantiated.
  for (const auto& kv : contexts_) {
    CHECK(!kv.second.instance);
  }
  base::ScopedAllowBlocking scoped_allow_blocking;

  const base::FilePath default_dir = AwBrowserContext::BuildStoragePath(
      base::FilePath(kDefaultContextPath));

  // ALOHA CU-86eqe7rd2: the old ALOHA cookie mechanism wrote the global
  // public/private cookie stores to <bromium_dir>/Cookies.{0,1}, where
  // <bromium_dir> depended on GetContextPath() for parent_context_==null. That
  // return value differed across historical builds:
  //  - <app_data>/bromium         (older builds — app-data root; verified on
  //                                device for the shipped version)
  //  - <app_data>/Default/bromium (builds where it returned the Default path)
  // Check both so upgrades from any old layout are migrated.
  const base::FilePath candidate_bromium_dirs[] = {
      AwBrowserContext::BuildStoragePath(base::FilePath(FILE_PATH_LITERAL("bromium"))),
      default_dir.Append(FILE_PATH_LITERAL("bromium")),
  };
  // Most recently used candidate for a given cookie filename. When a device
  // upgraded through both historical layouts, Cookies.<inst> may exist in BOTH
  // candidate dirs; pick the freshest (last modified = most recent session) so
  // the user keeps their latest cookies rather than the first candidate found.
  auto resolve_src = [&](const char* name) -> base::FilePath {
    base::FilePath best;
    base::Time best_mtime;
    for (const base::FilePath& dir : candidate_bromium_dirs) {
      base::FilePath p = dir.Append(name);
      base::File::Info info;
      if (base::GetFileInfo(p, &info)) {
        if (best.empty() || info.last_modified > best_mtime) {
          best = p;
          best_mtime = info.last_modified;
        }
      }
    }
    return best;
  };

  // ALOHA CU-86eqe7rd2: the old builds ALWAYS leave a stale (typically empty)
  // store at Default/Cookies — the unused CookieManager::GetDefaultInstance()
  // opens one on every start. So "dest exists" can NOT mean "already migrated";
  // inferring it that way skipped the Cookies.0 copy while the cleanup below
  // consumed the source, destroying the public session on every upgrade
  // (reproduced on-device, 2026-08-14). The completed-once signal is this
  // marker file instead. It is written synchronously between the copies and
  // the source cleanup (a pref would flush asynchronously and could lose that
  // ordering to a crash), so:
  //  - marker absent, sources present → upgrade, or a crash before the marker:
  //    copy the sources over whatever stale dest exists (re-copying identical
  //    sources is harmless — nothing writes to them between runs);
  //  - marker present, sources present → the cleanup crashed halfway or a
  //    residual layout resurfaced: never copy again, only consume leftovers.
  const base::FilePath migration_marker = default_dir.DirName().Append(
      FILE_PATH_LITERAL("bromium_cookies_migrated"));

  const base::FilePath public_src = resolve_src("Cookies.0");
  const base::FilePath private_src = resolve_src("Cookies.1");
  if (public_src.empty() && private_src.empty()) {
    // Fresh install, or the sources are already consumed. The consumed case
    // includes installs migrated by the previous (never-overwrite) build, which
    // knew no marker: stamp it retroactively, so a resurfaced residual
    // Cookies.{0,1} can never take the replace path over their live stores.
    if (!base::PathExists(migration_marker)) {
      base::WriteFile(migration_marker, "");
    }
    return;
  }

  if (!base::PathExists(migration_marker)) {
    // Guard: pre-init cookie API is forbidden by the AAR contract, but if a
    // provisional cookie store is nonetheless open — do not touch files under a
    // live SQLite, retry on the next start.
    if (CookieManager::AnyCookieStoreCreatedForMigration()) {
      return;
    }

    bool copied_ok = true;

    // Commit point of a single pair: once its dest is published, consume the
    // pair's sources in EVERY candidate dir right away. Without this, a
    // failure of a LATER pair would return with this pair's source still
    // present, and the next start would re-copy the old snapshot over a store
    // the session has meanwhile opened and written to — the very rollback the
    // marker-failure path below refuses to allow. Consuming residual copies in
    // the non-chosen layouts here matters too: resolve_src would otherwise
    // resurface one of them on the retry.
    const auto consume_sources =
        [&candidate_bromium_dirs](base::FilePath::StringViewType name) {
          for (const base::FilePath& dir : candidate_bromium_dirs) {
            bromium_migration::DeleteCookieStorePair(dir.Append(name));
          }
        };

    // Public: Cookies.0 → Default/Cookies.
    if (!public_src.empty()) {
      if (bromium_migration::MigrateCookieStorePair(
              public_src, default_dir.Append(FILE_PATH_LITERAL("Cookies")))) {
        consume_sources(FILE_PATH_LITERAL("Cookies.0"));
      } else {
        copied_ok = false;
      }
    }
    // Network state (Network Persistent State, TransportSecurity, ...) lived
    // next to the old cookies → Default/. Move from every candidate dir that
    // exists. Best-effort and self-consuming (base::Move), so not part of
    // copied_ok.
    for (const base::FilePath& dir : candidate_bromium_dirs) {
      if (base::PathExists(dir)) {
        bromium_migration::MoveBromiumNetworkStateFiles(dir, default_dir);
      }
    }

    // Private: Cookies.1 → <Profile N>/Cookies of the "Incognito" profile.
    if (!private_src.empty()) {
      Entry* entry;
      const auto it = contexts_.find(kIncognitoContextName);
      if (it != contexts_.end()) {
        // Repeat run after a crash between the prefs flush and the file move —
        // reuse the entry (a repeated CreateNewContext would CHECK-loop).
        entry = &it->second;
      } else {
        entry = CreateNewContext(kIncognitoContextName);
        // Best-effort expedite the profile.list write (ScopedListPrefUpdate in
        // CreateNewContext already marked prefs dirty). WITHOUT a blocking
        // RunLoop::Run() — here, in the store constructor inside
        // PreMainMessageLoopRun, the main message loop is not running yet, and
        // a nested RunLoop crashes (NOTREACHED in
        // ThreadControllerWithMessagePumpImpl::Run). Narrow window "crash after
        // move before prefs flush" → private cookies are not migrated once (the
        // user re-logs in private), not corruption.
        prefs_->CommitPendingWrite();
      }
      if (bromium_migration::MigrateCookieStorePair(
              private_src, AwBrowserContext::BuildStoragePath(entry->path)
                               .Append(FILE_PATH_LITERAL("Cookies")))) {
        consume_sources(FILE_PATH_LITERAL("Cookies.1"));
      } else {
        copied_ok = false;
      }
    }

    if (!copied_ok) {
      // A failed pair left its SOURCE and its dest DB untouched (see the
      // failure guarantee in the header: the dest's stale journal may already
      // be gone, which is fine for a marker-less disposable dest), so the next
      // start retries it from unchanged inputs. Pairs that succeeded are
      // already committed and their sources consumed above — the retry can no
      // longer re-copy them over a live store.
      return;
    }
    if (!base::WriteFile(migration_marker, "")) {
      // Do NOT return: the copies are already published, and once the cleanup
      // below consumes the sources the marker is only needed against
      // RESURFACED residuals (its absence alone triggers nothing — the
      // no-sources early return re-stamps it). Returning here instead would
      // open the worse window: a session writes to the live stores and the
      // next start's re-copy rolls them back to the old snapshot.
    }
  }

  // Consume ALL old sources in EVERY candidate dir — both the one just migrated
  // and any residual copy left in the non-chosen layout. Without this the
  // leftover Cookies.{0,1} survives and a later start would re-enter migration.
  // Ordering: the marker above is already on disk, so even if this cleanup is
  // interrupted, the next start takes the marker-present branch and only
  // deletes — it can never re-copy a stale residual over the live stores.
  for (const base::FilePath& dir : candidate_bromium_dirs) {
    bromium_migration::DeleteCookieStorePair(
        dir.Append(FILE_PATH_LITERAL("Cookies.0")));
    bromium_migration::DeleteCookieStorePair(
        dir.Append(FILE_PATH_LITERAL("Cookies.1")));
  }
}

bool AwBrowserContextStore::Exists(const std::string& name) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return contexts_.find(name) != contexts_.end();
}

std::vector<std::string> AwBrowserContextStore::List() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::vector<std::string> list;
  list.reserve(contexts_.size());
  for (const auto& kv : contexts_) {
    list.push_back(kv.first);
  }
  return list;
}

void AwBrowserContextStore::ForEachInstantiatedContext(
    base::FunctionRef<void(AwBrowserContext&)> callback) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (const auto& kv : contexts_) {
    if (kv.second.instance) {
      callback(*kv.second.instance);
    }
  }
}

AwBrowserContext* AwBrowserContextStore::Get(const std::string& name,
                                             const bool create_if_needed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  TRACE_EVENT("android_webview", "AwBrowserContextStore::Get", "name", name,
              "create_if_needed", create_if_needed);
  // Apps can specify a list of Profiles (BrowserContexts) to be initialized at
  // startup, meaning this can be called after thread restrictions are applied.
  base::ScopedAllowBlocking scoped_allow_blocking;
  auto context_it = contexts_.find(name);
  Entry* entry;
  std::optional<base::ScopedUmaHistogramTimer> histogram_timer;
  if (context_it != contexts_.end()) {
    entry = &context_it->second;
  } else {
    if (create_if_needed) {
      const bool is_default = name == kDefaultContextName;
      if (is_default) {
        // Default profile isn't explicitly listed in the
        // pref_store. Determining whether it exists already would require disk
        // accesses that might impact what we're trying to measure.
        histogram_timer.emplace(
            "Android.WebView.AwBrowserContext.Default.Duration."
            "CreateOrLoadFromDisk",
            base::ScopedUmaHistogramTimer::ScopedHistogramTiming::kShortTimes);
      } else {
        base::UmaHistogramLongTimes(
            "Android.WebView.AwBrowserContext.NonDefault.TimeSinceStartup."
            "Create",
            uptime_for_metrics_.Elapsed());
        histogram_timer.emplace(
            "Android.WebView.AwBrowserContext.NonDefault.Duration.Create",
            base::ScopedUmaHistogramTimer::ScopedHistogramTiming::kShortTimes);
      }
      entry = CreateNewContext(name);
    } else {
      return nullptr;
    }
  }
  if (!entry->instance) {
    const bool is_default = name == kDefaultContextName;
    if (!histogram_timer && !is_default) {
      base::UmaHistogramLongTimes(
          "Android.WebView.AwBrowserContext.NonDefault.TimeSinceStartup."
          "LoadFromDisk",
          uptime_for_metrics_.Elapsed());
      histogram_timer.emplace(
          "Android.WebView.AwBrowserContext.NonDefault.Duration.LoadFromDisk",
          base::ScopedUmaHistogramTimer::ScopedHistogramTiming::kShortTimes);
    }
    entry->instance =
        std::make_unique<AwBrowserContext>(name, entry->path, is_default);

    if (!is_default || base::FeatureList::IsEnabled(
                           features::kCreateSpareRendererForDefaultProfile)) {
      content::SpareRenderProcessHostManager::Get().WarmupSpare(
          entry->instance.get());
    }
    base::UmaHistogramCounts100(
        "Android.WebView.AwBrowserContext.Instantiations",
        ++instantiated_contexts_for_metrics_);
  }
  return entry->instance.get();
}

AwBrowserContextStore::DeletionResult AwBrowserContextStore::Delete(
    const std::string& name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto context_it = contexts_.find(name);
  if (context_it == contexts_.end()) {
    return DeletionResult::kDoesNotExist;
  }
  Entry* entry = &context_it->second;
  if (entry->instance) {
    return DeletionResult::kInUse;
  }

  std::optional<base::ScopedUmaHistogramTimer> histogram_timer;
  if (name != kDefaultContextName) {
    // As of writing, there is no way to delete the default profile as it is
    // unconditionally loaded, but this may change in future.
    base::UmaHistogramLongTimes(
        "Android.WebView.AwBrowserContext.NonDefault.TimeSinceStartup.Delete",
        uptime_for_metrics_.Elapsed());
    histogram_timer.emplace(
        "Android.WebView.AwBrowserContext.NonDefault.Duration.Delete",
        base::ScopedUmaHistogramTimer::ScopedHistogramTiming::kShortTimes);
  }
  ScopedListPrefUpdate update(&*prefs_, prefs::kProfileListPref);
  base::ListValue& profiles = update.Get();
  for (auto profile_it = profiles.begin(); profile_it != profiles.end();
       profile_it++) {
    const base::DictValue& dict = profile_it->GetDict();
    const std::string* cur_name = dict.FindString(kProfileNameKey);
    CHECK(cur_name);
    if (*cur_name == name) {
      const std::string* cur_path = dict.FindString(kProfilePathKey);
      CHECK(cur_path);
      CHECK_EQ(*cur_path, entry->path.value());
      // TODO(crbug.com/40268809): Make this async and backgroundable.
      AwBrowserContext::DeleteContext(entry->path);
      profiles.erase(profile_it);
      contexts_.erase(context_it);
      return DeletionResult::kDeleted;
    }
  }
  NOTREACHED() << "Profile exists in memory but not in prefs";
}

base::FilePath AwBrowserContextStore::GetRelativePathForTesting(
    const std::string& name) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto context_it = contexts_.find(name);
  CHECK(context_it != contexts_.end());
  return context_it->second.path;
}

AwBrowserContextStore::Entry* AwBrowserContextStore::CreateNewContext(
    std::string_view name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto emplace_result = contexts_.emplace(std::string(name), Entry());
  // Check it was new
  CHECK(emplace_result.second);
  Entry* entry = &emplace_result.first->second;

  ScopedListPrefUpdate update(&*prefs_, prefs::kProfileListPref);
  base::ListValue& profiles = update.Get();
  if (name == kDefaultContextName) {
    entry->path = base::FilePath(AwBrowserContextStore::kDefaultContextPath);
    AwBrowserContext::PrepareNewContext(entry->path);
    // Do not store the default profile in prefs - it is implicit.

  } else {
    int number = AssignNewProfileNumber();
    entry->path = base::FilePath(
        base::StrCat({"Profile ", base::NumberToString(number)}));
    AwBrowserContext::PrepareNewContext(entry->path);
    base::DictValue profileDict =
        base::DictValue()
            .Set(kProfileNameKey, name)
            .Set(kProfilePathKey, entry->path.value());
    profiles.Append(std::move(profileDict));
  }

  return entry;
}

int AwBrowserContextStore::AssignNewProfileNumber() {
  int number = prefs_->GetInteger(prefs::kProfileCounterPref) + 1;
  CHECK_GE(number, 1);
  prefs_->SetInteger(prefs::kProfileCounterPref, number);
  return number;
}

AwBrowserContext* AwBrowserContextStore::GetDefault() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!default_context_) {
    default_context_ = Get(kDefaultContextName, true);
  }
  return default_context_;
}

static bool JNI_AwBrowserContextStore_CheckNamedContextExists(
    JNIEnv* const env,
    const std::string& jname) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return AwBrowserContextStore::GetInstance()->Exists(jname);
}

static base::android::ScopedJavaLocalRef<jobject>
JNI_AwBrowserContextStore_GetNamedContextJava(JNIEnv* const env,
                                              const std::string& jname,
                                              bool create_if_needed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AwBrowserContext* context =
      AwBrowserContextStore::GetInstance()->Get(jname, create_if_needed);
  return context ? context->GetJavaBrowserContext() : nullptr;
}

static bool JNI_AwBrowserContextStore_DeleteNamedContext(
    JNIEnv* const env,
    const std::string& name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AwBrowserContextStore::DeletionResult result =
      AwBrowserContextStore::GetInstance()->Delete(name);
  switch (result) {
    case AwBrowserContextStore::DeletionResult::kDeleted:
      return true;
    case AwBrowserContextStore::DeletionResult::kDoesNotExist:
      return false;
    case AwBrowserContextStore::DeletionResult::kInUse:
      const std::string error_message =
          base::StrCat({"Cannot delete in-use profile ", name});
      env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                    error_message.c_str());
      return false;
  }
}

static std::string JNI_AwBrowserContextStore_GetNamedContextPathForTesting(
    JNIEnv* const env,
    const std::string& name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AwBrowserContextStore* store = AwBrowserContextStore::GetInstance();
  if (!store->Exists(name)) {
    return "";
  }
  base::FilePath path = store->GetRelativePathForTesting(name);
  return path.value();
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AwBrowserContextStore_ListAllContexts(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::vector<std::string> names =
      AwBrowserContextStore::GetInstance()->List();
  return base::android::ToJavaArrayOfStrings(env, names);
}

// static
void AwBrowserContextStore::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kProfileListPref, base::ListValue());
  registry->RegisterIntegerPref(prefs::kProfileCounterPref, 0);
}

// static
AwBrowserContextStore* AwBrowserContextStore::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(g_initialized);
  return GetOrCreateInstance();
}

// static
AwBrowserContextStore* AwBrowserContextStore::GetOrCreateInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  static base::NoDestructor<AwBrowserContextStore> instance(
      AwBrowserProcess::GetInstance()->local_state());
  // Relaxed: this is a one-way latch used only as a boolean "are we up" flag.
  // No reader depends on other memory published alongside it, so we need
  // atomicity (race-freedom) but not ordering. Compiles to a plain load/store.
  g_initialized.store(true, std::memory_order_relaxed);
  return instance.get();
}

// static
// ALOHA: thread-safe, no UI-thread assertion. See header.
bool AwBrowserContextStore::IsInitialized() {
  return g_initialized.load(std::memory_order_relaxed);
}

AwBrowserContextStore::Entry::Entry() = default;

AwBrowserContextStore::Entry::Entry(
    base::FilePath&& path,
    std::unique_ptr<AwBrowserContext>&& instance)
    : path(std::move(path)), instance(std::move(instance)) {}

AwBrowserContextStore::Entry::Entry(Entry&&) = default;

AwBrowserContextStore::Entry::~Entry() = default;

}  // namespace android_webview

DEFINE_JNI(AwBrowserContextStore)
