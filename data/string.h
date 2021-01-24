// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef string_h
#define string_h

#include <string_view>
#include "array.h"
#include "common/common.h"
#include "memory/handle.h"
#include "memory/ptr.h"

namespace codeswitch {
namespace internal {

/**
 * An immutable sequence of bytes representing UTF-8 encoded text.
 */
class String {
 public:
  void* operator new(size_t);
  String(const String& s);
  String(String&& s);
  String& operator=(const String& s);
  String& operator=(String&& s) = delete;
  static String* make(const char* s);
  static String* make(const std::string& s);

  length_t length() const { return length_; }
  String slice(length_t i, length_t j) const;
  std::string_view view() const;

  intptr_t compare(const String& r) const;
  intptr_t compare(const String* r) const { return compare(*r); }
  intptr_t compare(const char* r) const;

 private:
  String(length_t length, const Array<uint8_t>* data);

  length_t length_;
  Ptr<const Array<uint8_t>> data_;
};

class HashString {
 public:
  static word_t hash(const Ptr<String>& s);
  static bool equal(const Ptr<String>& l, const Ptr<String>& r) { return l->compare(*r) == 0; }
};

}  // namespace internal
}  // namespace codeswitch

#endif
