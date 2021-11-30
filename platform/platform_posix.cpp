// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "platform.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include "common/file.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

void* allocateChunk(size_t size, size_t alignment) {
  // TODO: randomize the allocation address.
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* basePtr = mmap(nullptr, size + alignment, prot, flags, -1, 0);
  if (basePtr == MAP_FAILED) {
    throw SystemAllocationError{errno};
  }
  auto base = reinterpret_cast<address>(basePtr);
  auto end = base + size + alignment;
  auto chunk = align(base, alignment);
  auto chunkEnd = chunk + size;
  if (chunk > base) {
    munmap(basePtr, chunk - base);
  }
  if (chunkEnd < end) {
    munmap(reinterpret_cast<void*>(chunkEnd), end - chunkEnd);
  }
  return reinterpret_cast<void*>(chunk);
}

void freeChunk(void* addr, size_t size) {
  munmap(addr, size);
}

MappedFile::MappedFile(const filesystem::path& filename, MappedFile::Perm perm) {
  auto openFlags = O_RDONLY;
  if (perm & Perm::WRITE) {
    openFlags = O_RDWR;
  }
  auto prot = 0;
  if (perm & READ) prot |= PROT_READ;
  if (perm & WRITE) prot |= PROT_WRITE;
  if (perm & EXEC) prot |= PROT_EXEC;
  auto mapFlags = MAP_SHARED | MAP_FILE;

  auto fd = open(filename.c_str(), openFlags);
  if (fd < 0) {
    throw FileError(filename, "could not open file");
  }
  struct stat st;
  auto ret = fstat(fd, &st);
  if (ret < 0) {
    close(fd);
    throw FileError(filename, "could not stat file");
  }
  if (static_cast<off_t>(static_cast<intptr_t>(st.st_size) != st.st_size)) {
    close(fd);
    throw FileError(filename, "file is too large to be memory-mapped into a 32-bit address space");
  }

  off_t offset = 0;
  auto addr = mmap(nullptr, st.st_size, prot, mapFlags, fd, offset);
  close(fd);
  if (addr == MAP_FAILED) {
    throw FileError(filename, "could not map file");
  }

  this->filename = filename;
  data = reinterpret_cast<uint8_t*>(addr);
  size = static_cast<uintptr_t>(st.st_size);
}

MappedFile::MappedFile(const filesystem::path& filename, size_t size, int mode) {
  auto openFlags = O_RDWR | O_CREAT;
  auto fd = open(filename.c_str(), openFlags, mode);
  if (fd < 0) {
    throw FileError(filename, "could not create file");
  }
  auto ret = ftruncate(fd, 0);
  if (ret < 0) {
    close(fd);
    throw FileError(filename, "could not delete existing file content");
  }
  ret = ftruncate(fd, size);
  if (ret < 0) {
    close(fd);
    throw FileError(filename, "could not resize file");
  }

  off_t offset = 0;
  auto prot = PROT_READ | PROT_WRITE;
  auto mapFlags = MAP_SHARED | MAP_FILE;
  auto addr = mmap(nullptr, size, prot, mapFlags, fd, offset);
  close(fd);
  if (addr == MAP_FAILED) {
    throw FileError(filename, "could not map file");
  }

  this->filename = filename;
  data = reinterpret_cast<uint8_t*>(addr);
  size = static_cast<uintptr_t>(size);
}

MappedFile::MappedFile(MappedFile&& file) : filename(file.filename), data(file.data), size(file.size) {
  file.filename = filesystem::path();
  file.data = nullptr;
  file.size = 0;
}

MappedFile& MappedFile::operator=(MappedFile&& file) {
  if (file.data) {
    munmap(file.data, file.size);
  }
  filename = file.filename;
  data = file.data;
  size = file.size;
  file.filename = filesystem::path();
  file.data = nullptr;
  file.size = 0;
  return *this;
}

MappedFile::~MappedFile() {
  if (data) {
    munmap(data, size);
  }
}

TempFile::TempFile(const std::string& pattern) {
  auto dir = filesystem::temp_directory_path();
  for (int i = 0; i < 1000; i++) {
    auto r = rand();
    auto rs = std::to_string(r);
    auto name = pattern;
    auto it = name.find("*");
    if (it == std::string::npos) {
      name += rs;
    } else {
      name.replace(it, 1, rs);
    }
    auto path = dir / name;

    auto fd = open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
      if (errno == EEXIST) {
        continue;
      }
      throw FileError(dir, "could not create temporary file");
    }
    close(fd);
    filename = path;
    return;
  }
  throw FileError(dir, "could not create temporary file");
}

TempFile::~TempFile() {
  filesystem::remove(filename);
}

}  // namespace codeswitch
