// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_h
#define common_h

#include <cstddef>
#include <cstdint>

namespace codeswitch {
namespace internal {

typedef uintptr_t word_t;
typedef uintptr_t address;
typedef uintptr_t length_t;

const word_t KB = 1 << 10;
const word_t MB = 1 << 20;
const word_t GB = 1 << 30;

const size_t kWordSize = sizeof(word_t);
const word_t kBitsInWord = kWordSize * 8;

constexpr inline word_t align(word_t n, word_t alignment) {
  return (n + alignment - 1UL) & ~(alignment - 1UL);
}

constexpr inline word_t alignDown(word_t n, word_t alignment) {
  return n & ~(alignment - 1UL);
}

constexpr inline word_t isAligned(word_t n, word_t alignment) {
  return (n & (alignment - 1UL)) == 0;
}

inline bool bit(word_t n, word_t bit) {
  return (n & (1UL << bit)) != 0;
}

inline word_t bitExtract(word_t n, word_t width, word_t shift) {
  return (n >> shift) & ((1UL << width) - 1UL);
}

inline word_t bitInsert(word_t n, word_t value, word_t width, word_t shift) {
  word_t mask = ((1UL << width) - 1UL) << shift;
  return (n & ~mask) | ((value << shift) & mask);
}

void abort(const char* fileName, int lineNumber, const char* reason, ...);

#define ASSERT(cond)                     \
  do {                                   \
    if (!(cond)) {                       \
      ABORT("assertion failed: " #cond); \
    }                                    \
  } while (0)

#define ABORT(reason) abort(__FILE__, __LINE__, (reason))

#define NON_COPYABLE(T)            \
  T(const T&) = delete;            \
  T& operator=(const T&) = delete; \
  T(T&&) = delete;                 \
  T& operator=(T&&) = delete;
}  // namespace internal
}  // namespace codeswitch

#endif
