// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_error_h
#define common_error_h

#include <exception>
#include <string>

namespace codeswitch {

class Error : public std::exception {
 public:
  explicit Error(const std::string& what) : what_(what) {}
  virtual const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};

Error errorf(const char* fmt, ...);

}  // namespace codeswitch

#endif
