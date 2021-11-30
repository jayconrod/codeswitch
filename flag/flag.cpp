// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "flag.h"

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/common.h"
#include "common/error.h"
#include "common/str.h"

namespace codeswitch {

void FlagSet::varFlag(const std::string& name, std::function<void(const std::string&)> parse,
                      const std::string& description, bool mandatory) {
  auto it = std::lower_bound(flags_.begin(), flags_.end(), name, FlagSpec::less);
  ASSERT(it == flags_.end() || it->name != name);
  flags_.insert(it, FlagSpec{name, parse, description, mandatory});
}

void FlagSet::stringFlag(std::string* value, const std::string& name, const std::string& defaultValue,
                         const std::string& description, bool mandatory) {
  *value = defaultValue;
  auto parse = [value](const std::string& arg) { *value = arg; };
  varFlag(name, parse, description, mandatory);
}

size_t FlagSet::parse(int argc, char* argv[]) {
  std::vector<std::string> args(argc);
  for (int i = 0; i < argc; i++) {
    args[i] = argv[i];
  }
  return parse(args);
}

class FlagError : public Error {
 public:
  FlagError(const std::string& name, const std::string& message) :
      Error(strprintf("%s: %s\n\tRun with -help for usage.", name.c_str(), message.c_str())) {}
};

size_t FlagSet::parse(const std::vector<std::string>& args) {
  for (size_t i = 0; i < args.size(); i++) {
    auto& arg = args[i];
    if (arg.size() < 2 || arg[0] != '-') {
      // Not a flag, must be the first positional argument.
      return i;
    }
    if (arg == "--") {
      // End flag flags. Positional arguments start after this.
      return i + 1;
    }
    auto nameBegin = arg[1] == '-' ? 2 : 1;
    auto name = arg.substr(nameBegin, arg.size() - nameBegin);
    auto eq = name.find("=");
    std::string value;
    if (eq != std::string::npos) {
      value = name.substr(eq + 1, name.size() - eq - 1);
      name = name.substr(0, eq);
    }
    auto it = std::lower_bound(flags_.begin(), flags_.end(), name, FlagSpec::less);
    if (it == flags_.end() || it->name != name) {
      throw FlagError(name, "no such flag");
    }
    if (eq == std::string::npos) {
      if (i + 1 >= args.size()) {
        throw FlagError(name, "expected argument with flag");
      }
      value = args[++i];
    }
    try {
      it->parse(value);
    } catch (const std::exception& ex) {
      throw FlagError(name, ex.what());
    }
  }
  return args.size();
}

void FlagSet::printUsage(std::ostream& out) {
  size_t maxNameLength = 0;
  for (auto& f : flags_) {
    if (f.name.size() > maxNameLength) {
      maxNameLength = f.name.size();
    }
  }

  out << "usage: " << programName_ << " " << shortUsage_ << "\n\n";
  for (auto& f : flags_) {
    out << "\n\t-" << f.name;
    for (size_t i = f.name.size(); i < maxNameLength; i++) {
      out << ' ';
    }
    out << '\t' << f.description;
  }
}

bool FlagSet::FlagSpec::less(const FlagSet::FlagSpec& l, const std::string& r) {
  return l.name < r;
}

}  // namespace codeswitch
