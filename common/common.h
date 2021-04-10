// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_common_h
#define common_common_h

#include <cstddef>
#include <cstdint>
#include <exception>

namespace codeswitch {

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

constexpr inline bool isPowerOf2(word_t n) {
  return n != 0 && (n & (n - 1)) == 0;
}

constexpr inline word_t nextPowerOf2(word_t n) {
  n += (n == 0);
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  if (sizeof(n) == 8) {
    n |= n >> 32;
  }
  n++;
  return n;
}

extern bool abortThrowException;
extern bool abortBacktrace;

class AbortError : public std::exception {
 public:
  virtual const char* what() const noexcept override { return message; }
  char message[2048];
};

void abort(const char* fileName, int lineNumber, const char* reason, ...);

#define ASSERT(cond)                     \
  do {                                   \
    if (!(cond)) {                       \
      ABORT("assertion failed: " #cond); \
    }                                    \
  } while (0)

#define ABORT(reason) abort(__FILE__, __LINE__, (reason))

#define UNREACHABLE() ABORT("unreachable")

#define USE(e) (void)(e)

#define CHECK_SUBTYPE_VALUE(type, value) \
  do {                                   \
    if (false) {                         \
      type _t = (value);                 \
      USE(_t);                           \
    }                                    \
  } while (0)

#define NON_COPYABLE(T)            \
  T(const T&) = delete;            \
  T& operator=(const T&) = delete; \
  T(T&&) = delete;                 \
  T& operator=(T&&) = delete;

const word_t kGarbageHandle = 0xDEADBEEFul;

}  // namespace codeswitch

#endif
