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

void* String::operator new(size_t size) {
  return heap.allocate(size);
}

String::String(length_t length, const Array<uint8_t>* data) : length_(length), data_(data) {}

String* String::make(const char* s) {
  length_t length = strlen(s);
  auto data = Array<uint8_t>::make(length);
  std::copy_n(s, length, reinterpret_cast<uint8_t*>(data));
  return new String(length, data);
}

String* String::make(const std::string& s) {
  auto data = Array<uint8_t>::make(s.size());
  std::copy(s.begin(), s.end(), reinterpret_cast<uint8_t*>(data));
  return new String(s.size(), data);
}

String::String(const String& s) : length_(s.length_), data_(s.data_.get()) {}

String::String(String&& s) : length_(s.length_), data_(s.data_.get()) {
  s.data_.set(nullptr);
}

String& String::operator=(const String& s) {
  length_ = s.length_;
  data_.set(s.data_.get());
  return *this;
}

String String::slice(length_t i, length_t j) const {
  if (j > length_ || j < i) {
    throw BoundsCheckError();
  }
  return String(j - i, data_.get()->slice(i));
}

std::string_view String::view() const {
  return std::string_view(reinterpret_cast<const char*>(&data_->at(0)), length_);
}

intptr_t String::compare(const String& r) const {
  if (this == &r) {
    return 0;
  }
  auto n = std::min(length_, r.length_);
  auto cmp = memcmp(data_.get(), r.data_.get(), n);
  if (cmp != 0) {
    return cmp;
  }
  auto ln = static_cast<intptr_t>(length_);
  auto rn = static_cast<intptr_t>(r.length_);
  return ln - rn;
}

intptr_t String::compare(const char* s) const {
  auto ld = data_.get();
  length_t i;
  for (i = 0; i < length_ && s[i] != '\0'; i++) {
    auto lb = static_cast<intptr_t>(ld->at(i));
    auto rb = static_cast<intptr_t>(s[i]);
    auto cmp = lb - rb;
    if (cmp != 0) {
      return cmp;
    }
  }
  if (i < length_) {
    return 1;
  } else if (s[i] != '\0') {
    return -1;
  } else {
    return 0;
  }
}

word_t HashString::hash(const Ptr<String>& s) {
  return std::hash<std::string_view>{}(s->view());
}

}  // namespace codeswitch
