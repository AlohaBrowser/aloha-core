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

#include "base/files/file_path.h"
#include "components/adblock/core/snippet/snippet_storage.h"
#include "components/prefs/pref_service.h"

#ifndef COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_STORAGE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_STORAGE_IMPL_H_

namespace adblock {

class SnippetStorageImpl final : public SnippetStorage {
 public:
  explicit SnippetStorageImpl(base::FilePath storage_dir);
  ~SnippetStorageImpl() override;

  const std::string& GetLoadedSnippets() const final;
  void LoadSnippets(const std::string& signature) final;
  void StoreSnippets(
      const std::string& content,
      base::OnceCallback<void(const std::string&)> callback) final;

 private:
  base::FilePath SnippetsLibraryFile() const;
  void OnSnippetsLoaded(const std::string& content);
  void OnSnippetsStored(const std::string& content,
                        base::OnceCallback<void(const std::string&)> callback,
                        bool success);

  base::FilePath storage_dir_;
  std::string snippets_lib_;

  base::WeakPtrFactory<SnippetStorageImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_STORAGE_IMPL_H_
