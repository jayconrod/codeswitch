// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_error_h
#define common_error_h

#include <exception>
#include <string>
#include "str.h"

namespace codeswitch {

class Error : public std::exception {
 public:
  explicit Error(const std::string& what) : what_(what) {}
  virtual const char* what() const noexcept override { return what_.c_str(); }

 protected:
  std::string what_;
};

template <class... Args>
Error errorstr(Args... args) {
  return Error(buildString(args...));
}

}  // namespace codeswitch

#endif
