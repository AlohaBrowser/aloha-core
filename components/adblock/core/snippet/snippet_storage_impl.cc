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

#include "components/adblock/core/snippet/snippet_storage_impl.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "ui/base/resource/resource_bundle.h"

namespace adblock {
namespace {

char kSnippetsFileName[] = "snippet.js";

std::string LoadLocalSnippets(const std::string& signature,
                              const base::FilePath& storage_dir,
                              const base::FilePath& snippets_lib_file) {
  if (!base::PathExists(storage_dir)) {
    LOG(WARNING) << "[eyeo] Storage for snippet library to read does not exist";
    return {};
  }
  std::string content;
  if (!base::ReadFileToString(snippets_lib_file, &content)) {
    LOG(WARNING) << "[eyeo] Cannot read snippet library file";
    return {};
  }
  auto current_signature = SubscriptionValidatorImpl::ComputeSubscriptionHash(
      InMemoryFlatbufferData(content));
  if (current_signature != signature) {
    LOG(ERROR) << "[eyeo] Unexpected signature: wanted " << signature
               << " but found " << current_signature
               << ". Falling back to prebundled version.";
    return {};
  }
  return content;
}

bool SaveSnippetsToFile(const base::FilePath& path,
                        const base::FilePath& snippets_lib_file,
                        const std::string& content) {
  if (!base::CreateDirectory(path)) {
    LOG(WARNING) << "[eyeo] Cannot create storage dir for snippet library";
    return false;
  }
  if (!base::WriteFile(snippets_lib_file, content)) {
    LOG(WARNING) << "[eyeo] Cannot write snippet library file";
    return false;
  }
  return true;
}
}  // namespace

SnippetStorageImpl::SnippetStorageImpl(base::FilePath storage_dir)
    : storage_dir_(storage_dir) {}

SnippetStorageImpl::~SnippetStorageImpl() = default;

void SnippetStorageImpl::LoadSnippets(const std::string& signature) {
  if (!snippets_lib_.empty()) {
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&LoadLocalSnippets, signature, storage_dir_,
                     SnippetsLibraryFile()),
      base::BindOnce(&SnippetStorageImpl::OnSnippetsLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SnippetStorageImpl::OnSnippetsLoaded(const std::string& content) {
  snippets_lib_ =
      content.empty()
          ? ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
                IDR_ADBLOCK_SNIPPETS_JS)
          : content;
}

const std::string& SnippetStorageImpl::GetLoadedSnippets() const {
  return snippets_lib_;
}

void SnippetStorageImpl::StoreSnippets(
    const std::string& content,
    base::OnceCallback<void(const std::string&)> callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SaveSnippetsToFile, storage_dir_, SnippetsLibraryFile(),
                     content),
      base::BindOnce(&SnippetStorageImpl::OnSnippetsStored,
                     weak_ptr_factory_.GetWeakPtr(), content,
                     std::move(callback)));
}

void SnippetStorageImpl::OnSnippetsStored(
    const std::string& content,
    base::OnceCallback<void(const std::string&)> callback,
    bool success) {
  if (!success) {
    std::move(callback).Run({});
    return;
  }
  snippets_lib_ = content;
  auto signature = SubscriptionValidatorImpl::ComputeSubscriptionHash(
      InMemoryFlatbufferData(content));
  std::move(callback).Run(signature);
}

base::FilePath SnippetStorageImpl::SnippetsLibraryFile() const {
  return storage_dir_.AppendASCII(kSnippetsFileName);
}

}  // namespace adblock
