// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "interpreter.h"

#include <filesystem>
#include <regex>
#include <sstream>
#include "common/file.h"
#include "memory/handle.h"
#include "package/asm.h"
#include "test/test.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

// For each .csws file in the testdata directory, assemble the file, interpret
// it, then ensure printed values match expected values written in comments.
TEST(Assembly) {
  filesystem::path path("interpreter/testdata");
  std::basic_regex outputRe("(?:^|\\n).*?//\\s*Output:\\s*(.*)(?:$|\\n)");
  for (filesystem::directory_iterator it(path); it != filesystem::directory_iterator(); it++) {
    auto filename = it->path();
    if (filename.extension() != ".csws") {
      continue;
    }
    auto package = readPackageAsm(filename);
    auto name = handle(String::make("main"));
    auto entry = handle(package->findFunction(name.get()));
    ASSERT_TRUE(entry);

    std::stringstream ss;
    interpret(package, entry, ss);
    auto got = ss.str();

    ss = std::stringstream();
    auto data = readFile(filename);
    auto pos = reinterpret_cast<const char*>(&data[0]);
    auto end = pos + data.size();
    std::match_results<const char*> m;
    while (std::regex_search(pos, end, m, outputRe)) {
      ss << m[1].str() << std::endl;
      pos = m.suffix().first;
    }
    auto want = ss.str();
    ASSERT_EQ(got, want);
  }
}

}  // namespace codeswitch
