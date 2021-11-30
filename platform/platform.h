// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef platform_platform_h
#define platform_platform_h

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include "common/common.h"

namespace codeswitch {

class SystemAllocationError : public std::exception {
 public:
  explicit SystemAllocationError(int err) : err_(err) {}
  virtual const char* what() const noexcept override { return strerror(err_); }

 private:
  int err_;
};

/** Allocates a region of memory from the kernel with the given size and * alignment. */
void* allocateChunk(size_t size, size_t alignment);

/** Frees a region allocated with {@code allocateChunk}. */
void freeChunk(void* addr, size_t size);

class MappedFile {
 public:
  enum Perm {
    EXEC = 1,
    WRITE = 2,
    READ = 4,
  };

  MappedFile() : data(nullptr), size(0) {}
  MappedFile(const std::filesystem::path& filename, Perm perm);
  MappedFile(const std::filesystem::path& filename, size_t size, int mode);
  MappedFile(const MappedFile&) = delete;
  MappedFile(MappedFile&& file);
  MappedFile& operator=(const MappedFile&) = delete;
  MappedFile& operator=(MappedFile&& file);
  ~MappedFile();

  std::filesystem::path filename;
  uint8_t* data;
  uintptr_t size;

  uint8_t* end() { return data + size; }
};

class TempFile {
 public:
  explicit TempFile(const std::string& pattern);
  ~TempFile();

  std::filesystem::path filename;
};

}  // namespace codeswitch

#endif
