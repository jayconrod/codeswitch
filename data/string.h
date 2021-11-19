// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef data_string_h
#define data_string_h

#include <string_view>
#include "array.h"
#include "common/common.h"
#include "memory/handle.h"
#include "memory/ptr.h"

namespace codeswitch {

/**
 * A sequence of bytes representing UTF-8 text.
 *
 * String is essentially a wrapper around BoundArray, so it's important not
 * to pass by value in C++ code, since that will execute write barriers
 * on the C++ stack, which is not allowed. Strings generally should be passed
 * by value in managed code though.
 *
 * The bytes referenced by a String are immutable, though the string header
 * itself is not.
 */
class String {
 public:
  static String* make();
  static String* make(BoundArray<const uint8_t>& data);
  static String* make(Array<const uint8_t>* array, length_t length);
  static Handle<String> create(const char* s);
  static Handle<String> create(const std::string& s);
  static Handle<String> create(const std::string_view& s);

  length_t length() const { return data_.length(); }
  std::string_view view() const;
  const uint8_t* begin() const { return data_.begin(); }
  const uint8_t* end() const { return data_.end(); }

  void slice(length_t i, length_t j);

  intptr_t compare(const String& r) const;
  intptr_t compare(const char* r) const;
  intptr_t compare(const std::string& s) const;
  intptr_t compare(const std::string_view& s) const;
  bool operator==(const String& s) const { return compare(s) == 0; }
  bool operator!=(const String& s) const { return compare(s) != 0; }
  bool operator<(const String& s) const { return compare(s) < 0; }
  bool operator<=(const String& s) const { return compare(s) <= 0; }
  bool operator>(const String& s) const { return compare(s) > 0; }
  bool operator>=(const String& s) const { return compare(s) >= 0; }

 private:
  friend std::ostream& operator<<(std::ostream&, const String&);

  String() = default;
  explicit String(BoundArray<const uint8_t>& data) : data_(data) {}
  String(Array<const uint8_t>* array, length_t length) : data_(array, length) {}

  BoundArray<const uint8_t> data_;
};

class HashString {
 public:
  static word_t hash(const String& s);
  static bool equal(const String& l, const String& r) { return l.compare(r) == 0; }
};

std::ostream& operator<<(std::ostream&, String*);

}  // namespace codeswitch

#endif
