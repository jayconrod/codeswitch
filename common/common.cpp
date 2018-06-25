// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "common.h"

#include <execinfo.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

namespace codeswitch {
namespace internal {

bool abortThrowException = false;
bool abortBacktrace = false;

void abort(const char* fileName, int lineNumber, const char* reason, ...) {
  va_list ap;
  va_start(ap, reason);
  AbortError err;
  size_t n = snprintf(err.message, sizeof(err.message), "%s: %d: ", fileName, lineNumber);
  n += vsnprintf(err.message + n, sizeof(err.message) - n, reason, ap);
  va_end(ap);
  if (abortBacktrace && n < sizeof(err.message)) {
    void* frames[100];
    auto frameCount = backtrace(frames, sizeof(frames) / sizeof(frames[0]));
    auto symbols = backtrace_symbols(frames, frameCount);
    for (int i = 0; i < frameCount && n < sizeof(err.message); i++) {
      n += snprintf(err.message + n, sizeof(err.message) - n, "\n%s", symbols[i]);
    }
    free(symbols);
  }

  if (abortThrowException) {
    throw err;
  } else {
    cerr << err.message << endl;
    ::abort();
  }
}
}  // namespace internal
}  // namespace codeswitch
