/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "content/shell/browser/adblock/adblock_shell_browser_context.h"

#include "components/adblock/core/common/adblock_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_name_set.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/segregated_pref_store.h"
#include "components/user_prefs/user_prefs.h"

namespace content {

AdblockShellBrowserContext::AdblockShellBrowserContext()
    : ShellBrowserContext(false) {
  CreateUserPrefService();
}

AdblockShellBrowserContext::~AdblockShellBrowserContext() {}

bool AdblockShellBrowserContext::IsOffTheRecord() {
  // Adblock services should not be created for off-the-record contexts
  return false;
}

void AdblockShellBrowserContext::CreateUserPrefService() {
  auto pref_registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();

  adblock::common::prefs::RegisterProfilePrefs(pref_registry.get());

  PrefServiceFactory pref_service_factory;

  PrefNameSet persistent_prefs;

  // These prefs go in the JsonPrefStore, and will persist across runs.
  for (auto& pref_name : adblock::common::prefs::GetPrefs()) {
    persistent_prefs.insert(pref_name.data());
  }

  pref_service_factory.set_user_prefs(base::MakeRefCounted<SegregatedPrefStore>(
      base::MakeRefCounted<InMemoryPrefStore>(),
      base::MakeRefCounted<JsonPrefStore>(
          GetPath().Append(FILE_PATH_LITERAL("Preferences"))),
      std::move(persistent_prefs)));

  user_pref_service_ = pref_service_factory.Create(pref_registry);

  user_prefs::UserPrefs::Set(this, user_pref_service_.get());
}

}  // namespace content
