// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "runner.h"

#include <functional>
#include <thread>

namespace codeswitch {

void Runner::run(std::function<void()>&& task) {
  // TODO: implement M:N threading.
  //
  // We can get the number of machine threads with
  // std::thread::hardware_concurrency.
  //
  // However, we need a way for a thread to release its resources (allowing
  // other threads to spawn) when it's blocked, for example, on a lock or
  // on I/O.
  new std::thread(task);
}

}  // namespace codeswitch
