// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef runner_runner_h
#define runner_runner_h

#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include "common/common.h"

namespace codeswitch {
namespace internal {

/**
 * Runner executes tasks asynchronously on a set of background threads.
 * 
 * Currently, this is a stub implementation that spawns a new thread for each
 * task. Later, Runner will implement M:N threading.
 */
class Runner {
 public:
  NON_COPYABLE(Runner)
  Runner();

  void run(std::function<void()>&& task);
};

extern Runner runner;

}  // namespace internal
}  // namespace codeswitch

#endif
