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

#include "components/adblock/core/common/flatbuffer_data.h"

#include "absl/types/optional.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace adblock {
namespace {

// Destroys memory mapped to a file on disk, and optionally removes the file
// itself. Performs blocking operations, must run on a MayBlock() task runner.
void DestroyMemoryMappedFile(std::unique_ptr<base::MemoryMappedFile> memory,
                             absl::optional<base::FilePath> path_to_remove) {
  memory.reset();
  // Deleting the file should happen *after* removing the memory mapping.
  if (path_to_remove) {
    base::DeleteFile(*path_to_remove);
  }
}

}  // namespace

InMemoryFlatbufferData::InMemoryFlatbufferData(std::string data)
    : data_(std::move(data)) {}

InMemoryFlatbufferData::~InMemoryFlatbufferData() = default;

const uint8_t* InMemoryFlatbufferData::data() const {
  return reinterpret_cast<const uint8_t*>(data_.data());
}

size_t InMemoryFlatbufferData::size() const {
  return data_.size();
}

MemoryMappedFlatbufferData::MemoryMappedFlatbufferData(base::FilePath path)
    : permanently_remove_path_(false), path_(std::move(path)) {
  file_ = std::make_unique<base::MemoryMappedFile>();
  if (!file_->Initialize(path_)) {
    file_.reset();
  }
}

MemoryMappedFlatbufferData::~MemoryMappedFlatbufferData() {
  const auto path_to_remove =
      permanently_remove_path_.load()
          ? absl::optional<base::FilePath>(std::move(path_))
          : absl::nullopt;
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DestroyMemoryMappedFile, std::move(file_),
                     std::move(path_to_remove)));
}

const uint8_t* MemoryMappedFlatbufferData::data() const {
  if (!file_) {
    return nullptr;
  }
  return file_->data();
}

size_t MemoryMappedFlatbufferData::size() const {
  if (!file_) {
    return 0u;
  }
  return file_->length();
}

void MemoryMappedFlatbufferData::PermanentlyRemoveSourceOnDestruction() {
  permanently_remove_path_.store(true);
}

}  // namespace adblock
