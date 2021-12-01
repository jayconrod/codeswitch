// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "string.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string_view>
#include "array.h"
#include "memory/handle.h"
#include "memory/heap.h"

namespace codeswitch {

String* String::make() {
  return new (heap->allocate(sizeof(String))) String();
}

String* String::make(BoundArray<const uint8_t>& data) {
  return new (heap->allocate(sizeof(String))) String(data);
}

String* String::make(Array<const uint8_t>* array, size_t length) {
  return new (heap->allocate(sizeof(String))) String(array, length);
}

Handle<String> String::create(const char* s) {
  size_t length = strlen(s);
  auto array = handle(Array<uint8_t>::make(length));
  std::copy_n(s, length, array->begin());
  return handle(String::make(reinterpret_cast<Array<const uint8_t>*>(*array), length));
}

Handle<String> String::create(const std::string& s) {
  auto array = handle(Array<uint8_t>::make(s.size()));
  std::copy(s.begin(), s.end(), array->begin());
  return handle(String::make(reinterpret_cast<Array<const uint8_t>*>(*array), s.size()));
}

Handle<String> String::create(const std::string_view& s) {
  auto array = handle(Array<uint8_t>::make(s.size()));
  std::copy(s.begin(), s.end(), array->begin());
  return handle(String::make(reinterpret_cast<Array<const uint8_t>*>(*array), s.size()));
}

std::string_view String::view() const {
  return std::string_view(reinterpret_cast<const char*>(data_.begin()), length());
}

std::string String::str() const {
  return std::string(reinterpret_cast<const char*>(data_.begin()), length());
}

void String::slice(size_t i, size_t j) {
  data_.slice(i, j);
}

intptr_t String::compare(const String& r) const {
  if (this == &r) {
    return 0;
  }
  auto n = std::min(length(), r.length());
  auto cmp = memcmp(data_.begin(), r.data_.begin(), n);
  if (cmp != 0) {
    return cmp;
  }
  return length() - r.length();
}

intptr_t String::compare(const char* r) const {
  auto l = data_.begin();
  size_t i;
  for (i = 0; i < length() && r[i] != '\0'; i++) {
    auto lb = static_cast<intptr_t>(l[i]);
    auto rb = static_cast<intptr_t>(r[i]);
    auto cmp = lb - rb;
    if (cmp != 0) {
      return cmp;
    }
  }
  if (i < length()) {
    return 1;
  } else if (r[i] != '\0') {
    return -1;
  } else {
    return 0;
  }
}

intptr_t String::compare(const std::string& s) const {
  return compare(std::string_view(s));
}

intptr_t String::compare(const std::string_view& s) const {
  return std::lexicographical_compare(data_.begin(), data_.end(), s.begin(), s.end());
}

uintptr_t HashString::hash(const String& s) {
  return std::hash<std::string_view>{}(s.view());
}

std::ostream& operator<<(std::ostream& os, const String& s) {
  os.write(reinterpret_cast<const char*>(s.begin()), s.length());
  return os;
}

}  // namespace codeswitch
