// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_common_h
#define common_common_h

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace codeswitch {

const uintptr_t KB = 1 << 10;
const uintptr_t MB = 1 << 20;
const uintptr_t GB = 1 << 30;

const size_t kWordSize = sizeof(uintptr_t);
const uintptr_t kBitsInWord = kWordSize * 8;

constexpr inline uintptr_t align(uintptr_t n, uintptr_t alignment) {
  return (n + alignment - 1UL) & ~(alignment - 1UL);
}

constexpr inline uintptr_t alignDown(uintptr_t n, uintptr_t alignment) {
  return n & ~(alignment - 1UL);
}

constexpr inline uintptr_t isAligned(uintptr_t n, uintptr_t alignment) {
  return (n & (alignment - 1UL)) == 0;
}

inline bool bit(uintptr_t n, uintptr_t bit) {
  return (n & (1UL << bit)) != 0;
}

inline uintptr_t bitExtract(uintptr_t n, uintptr_t width, uintptr_t shift) {
  return (n >> shift) & ((1UL << width) - 1UL);
}

inline uintptr_t bitInsert(uintptr_t n, uintptr_t value, uintptr_t width, uintptr_t shift) {
  uintptr_t mask = ((1UL << width) - 1UL) << shift;
  return (n & ~mask) | ((value << shift) & mask);
}

constexpr inline bool isPowerOf2(uintptr_t n) {
  return n != 0 && (n & (n - 1)) == 0;
}

constexpr inline uintptr_t nextPowerOf2(uintptr_t n) {
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

template <class T>
constexpr bool addWouldOverflow(T a, T b) {
  if (std::is_unsigned<T>::value || (a > 0 && b > 0)) {
    return std::numeric_limits<T>::max() - b < a;
  }
  if (a < 0 && b < 0) {
    return std::numeric_limits<T>::min() - b > a;
  }
  return false;
}

template <class S, class T>
S narrow(T t) {
  auto s = static_cast<S>(t);
  if (static_cast<T>(s) != t) {
    throw std::domain_error("could not precisely cast integer to narrower type");
  }
  return s;
}

template <class T>
T readBin(uint8_t** p) {
  T v = *reinterpret_cast<T*>(*p);
  *p += sizeof(T);
  return v;
}

template <class T>
void readBin(uint8_t** p, T* v) {
  *v = *reinterpret_cast<T*>(*p);
  *p += sizeof(T);
}

template <class T>
void writeBin(uint8_t** p, T v) {
  *reinterpret_cast<T*>(*p) = v;
  *p += sizeof(T);
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

#define UNIMPLEMENTED() ABORT("unimplemented")

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

const uintptr_t kGarbageHandle = 0xDEADBEEFul;

}  // namespace codeswitch

#endif
