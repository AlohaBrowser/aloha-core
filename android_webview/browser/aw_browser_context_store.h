// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_BROWSER_CONTEXT_STORE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_BROWSER_CONTEXT_STORE_H_

#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/no_destructor.h"
#include "base/timer/elapsed_timer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace android_webview {

namespace prefs {

// List of profile dictionaries.
constexpr char kProfileListPref[] = "profile.list";

// Counter for assigning profile numbers (without reuse). Stores the last
// assigned number, or 0 if never used, which is the same as the total number of
// non-default profiles ever created.
constexpr char kProfileCounterPref[] = "profile.counter";

}  // namespace prefs

class AwBrowserContext;

// AwBrowserContextStore is the container for AwBrowserContexts.
//
// - Owns and manages the lifetimes of instantiated AwBrowserContexts
//   ("Profiles").
//
// - Keeps track of the profiles instantiated into memory and the profiles saved
//   to disk (via the local_state pref_service), including mappings between
//   profile names and directory names.
//
// Most profiles are created and instantiated lazily (via the Get method).
// However, there is a special "Default" profile which is always created and
// instantiated on startup.
// Lifetime: Singleton
// ALOHA CU-86eqe7rd2 (U2): helpers for the one-shot migration of public/private
// mode cookie files to the profile paths. Exposed as free functions for
// unit tests (see aw_browser_context_store_unittest.cc).
namespace bromium_migration {

// Copies a pair of SQLite files (db + rollback journal) src_db → dest_db,
// REPLACING any existing dest pair. The old (pre-profile) builds always leave a
// stale store at Default/Cookies — the unused default cookie manager opens it
// on every start — so an existing dest means "old-version artifact", not "live
// migrated store". The caller is responsible for never invoking this after a
// completed migration (see the marker file in MigrateBromiumModeCookies); under
// that contract replacing dest is safe, and re-running after a crash re-copies
// identical bytes. Any pre-existing dest journal is deleted even when the
// source has no journal: SQLite pairs a rollback journal with its db by file
// name alone and would replay a foreign hot journal into the copied db,
// corrupting it. The source is left in place (the caller consumes a pair's
// sources right after its publish succeeds, via DeleteCookieStorePair). A
// missing src_db is a no-op. The copy is staged into *.migrating temp files
// and published via rename — journal first, the db rename last as the commit
// point.
//
// Failure guarantee (returns false), stated precisely: the SOURCE pair and the
// pre-existing dest DB file are untouched, but the dest's stale rollback
// journal may already have been deleted (dropping it is the first publish
// step). This is deliberate and safe under the marker contract above — a dest
// pair existing while the migration marker is absent is a disposable
// pre-profile artifact, not user data — but it is NOT a full "dest untouched"
// guarantee. The caller must keep the sources and retry on the next start.
bool MigrateCookieStorePair(const base::FilePath& src_db,
                            const base::FilePath& dest_db);

// Deletes a cookie store pair (db + rollback journal). Used to consume ALL old
// candidate sources after migration so no residual Cookies.{0,1} can re-trigger
// migration on a later start and overwrite live data.
void DeleteCookieStorePair(const base::FilePath& db);

// Moves network-state files (Network Persistent State, TransportSecurity, etc.)
// from the old bromium/ dir to Default/ — after reverting the ALOHA cookie
// mechanism the network context's data_directory returns to Default/. Skips
// Cookies.* (migrated separately) and subdirectories (bromium/TemporaryDownloads
// lives its own life). Unlike MigrateCookieStorePair, an existing dest is never
// overwritten (the source is dropped instead): network state is best-effort and
// losing stale HSTS/session-ticket data is preferable to clobbering live state.
void MoveBromiumNetworkStateFiles(const base::FilePath& bromium_dir,
                                  const base::FilePath& default_dir);

}  // namespace bromium_migration

class AwBrowserContextStore final {
 public:
  static constexpr char kDefaultContextName[] = "Default";
  static constexpr char kDefaultContextPath[] = "Default";
  // ALOHA CU-86eqe7rd2: private-mode profile name. Registered by the U2
  // migration (if private cookies existed) or by the Kotlin layer via the
  // Profile API on first entry into private mode.
  static constexpr char kIncognitoContextName[] = "Incognito";

  enum class DeletionResult {
    kDeleted,
    kDoesNotExist,
    kInUse,
  };

  ~AwBrowserContextStore() = delete;
  AwBrowserContextStore(const AwBrowserContextStore&) = delete;
  AwBrowserContextStore& operator=(const AwBrowserContextStore&) = delete;

  // Initialize the store if needed and then get a pointer to it.
  static AwBrowserContextStore* GetOrCreateInstance();
  // CHECK the store is initialized and then get a pointer to it.
  static AwBrowserContextStore* GetInstance();
  // ALOHA: thread-safe check whether the store has been initialized, without
  // the DCHECK_CURRENTLY_ON(UI) assertion of GetInstance(). Used as a
  // "running inside the browser" guard from non-UI threads.
  static bool IsInitialized();

  // Get the default context. This will never return null. Must be called on the
  // UI thread.
  AwBrowserContext* GetDefault();
  // Check if a named context exists (on disk or in memory). The context does
  // not need to be instantiated. The result is given to the callback once
  // determined.
  bool Exists(const std::string& name) const;
  // Instantiate and get the context with the given name. If the context does
  // not already exist (on disk or in memory), it is created if create_if_needed
  // is true, otherwise no context is instantiated and nullptr is returned.
  AwBrowserContext* Get(const std::string& name, bool create_if_needed);
  // Get a list of all contexts' names (on disk and in memory).
  std::vector<std::string> List() const;

  // ALOHA CU-86eqe7rd2 (U3/U4): apply the callback to every ALREADY
  // instantiated context (including Default, if created). Unlike
  // Get(name, false), does NOT instantiate profiles from disk as a side effect —
  // safe for sweep paths (DNT, flush prefs) without extra blocking IO and
  // WarmupSpare.
  void ForEachInstantiatedContext(
      base::FunctionRef<void(AwBrowserContext&)> callback) const;
  // Delete the named context if possible.
  DeletionResult Delete(const std::string& name);

  // Get the relative path for the named context. The context must exist (on
  // disk or in memory), but does not need to be instantiated.
  base::FilePath GetRelativePathForTesting(const std::string& name) const;

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AwBrowserContextStore>;

  struct Entry {
    Entry();
    explicit Entry(base::FilePath&& path,
                   std::unique_ptr<AwBrowserContext>&& instance);
    ~Entry();
    Entry(Entry&&);
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;

    base::FilePath path;
    std::unique_ptr<AwBrowserContext> instance;
  };

  // Create the global store of all AwBrowserContexts. The default context is
  // pre-loaded into the store, which may involve blocking IO.
  explicit AwBrowserContextStore(PrefService* pref_service);

  // Registers a new (default or non-default) context with the given name and
  // ensures filesystem state is ready for the context to be
  // initialized. Returns a pointer to the entry inserted into contexts_. This
  // does not instantiate the context in-process.
  //
  // It is illegal to create a context which already exists.
  Entry* CreateNewContext(std::string_view name);

  // ALOHA CU-86eqe7rd2 (U2): one-shot migration of the ALOHA-mode cookie files
  // (<app_data>/bromium/Cookies.0 → Default/Cookies, <app_data>/bromium/Cookies.1
  // → <Profile N>/Cookies of the "Incognito" profile) to the upstream paths.
  // Called at the end of the constructor — before Default is instantiated and
  // the cookie store is opened. Crash-atomicity order: register "Incognito" and
  // best-effort schedule the profile.list write BEFORE moving the files (a
  // repeated CreateNewContext would CHECK-loop; an orphaned move would lose the
  // private cookies).
  void MigrateBromiumModeCookies();
  // Finds and reserves a non-default profile number which hasn't previously
  // been assigned to any profile (even deleted ones).
  int AssignNewProfileNumber();

  const base::ElapsedTimer uptime_for_metrics_;
  size_t instantiated_contexts_for_metrics_ = 0;

  // (local_state) PrefService which is used to persist information about the
  // profiles that exist on disk.
  const raw_ref<PrefService> prefs_;

  // Map of all existing (on disk and in memory) profiles, including ones which
  // haven't been instantiated.
  std::map<std::string, Entry> contexts_;
  // The default context is initialized during store construction. Once the
  // store is created, this pointer will be non-null.
  raw_ptr<AwBrowserContext> default_context_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_BROWSER_CONTEXT_STORE_H_
