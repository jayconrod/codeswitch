// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "common.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

namespace codeswitch {
namespace internal {

void abort(const char* fileName, int lineNumber, const char* reason, ...) {
  va_list ap;
  va_start(ap, reason);
  char buffer[512];
  snprintf(buffer, sizeof(buffer), "%s: %d: ", fileName, lineNumber);
  cerr << buffer;
  vsnprintf(buffer, sizeof(buffer), reason, ap);
  cerr << buffer << endl;
  va_end(ap);
  ::abort();
}
}
}
