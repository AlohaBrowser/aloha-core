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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_FLATBUFFER_DATA_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_FLATBUFFER_DATA_H_

#include <atomic>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"

namespace adblock {

// Holds raw flatbuffer data.
// All methods must be thread-safe, the object can be accessed from multiple
// task runners concurrently.
class FlatbufferData {
 public:
  virtual ~FlatbufferData() = default;

  virtual const uint8_t* data() const = 0;
  virtual size_t size() const = 0;

  // Schedules permanent removal of the data source of this flatbuffer when
  // |this| is destroyed. This can mean removing a file from disk or removing
  // a record from a database etc.
  virtual void PermanentlyRemoveSourceOnDestruction() {}
};

// Implementation that loads the flatbuffer into memory from a source string.
// Requires around 5-10 MB of memory for a subscription like EasyList.
class InMemoryFlatbufferData : public FlatbufferData {
 public:
  explicit InMemoryFlatbufferData(std::string data);
  ~InMemoryFlatbufferData() override;
  const uint8_t* data() const override;
  size_t size() const override;

 private:
  std::string data_;
};

// Memory-mapped implementation that opens a file and memory-maps it. Should
// use less memory than InMemoryFlatbufferData because the bulk of the data
// resides on disk. Some memory is still consumed due to caching/paging and
// the application's *shared* memory usage may increase.
class MemoryMappedFlatbufferData : public FlatbufferData {
 public:
  // Ctor should be called on blocking task runner, performs file I/O/
  explicit MemoryMappedFlatbufferData(base::FilePath path);
  ~MemoryMappedFlatbufferData() override;
  const uint8_t* data() const override;
  size_t size() const override;

  // Post cleanup of the mapped file to blocking task runner during destruction.
  void PermanentlyRemoveSourceOnDestruction() final;

 private:
  // Since buffers may be accessed by many threads,
  // PermanentlyRemoveSourceOnDestruction() must set the cleanup flag
  // atomically.
  std::atomic_bool permanently_remove_path_;
  const base::FilePath path_;
  std::unique_ptr<base::MemoryMappedFile> file_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_FLATBUFFER_DATA_H_
