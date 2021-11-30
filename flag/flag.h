// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef flag_h
#define flag_h

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace codeswitch {

/**
 * Parses command-line flags and prints usage information for command-line
 * tools.
 *
 * Flags on the command-line may be of the form '-key=value' or '-key value'.
 * Flags may start with '-' or '--'. If an argument is not a value for a flag
 * or is '--', no more flags are processed.
 *
 * Flags may be registered with varFlag and similar methods. After all flags
 * are registered, parse should be called on the command-line arguments.
 * parse throws an exception if flags can't be parsed for some reason.
 */
class FlagSet {
 public:
  FlagSet(const std::string& programName, const std::string& shortUsage) :
      programName_(programName), shortUsage_(shortUsage) {}

  enum class Opt : bool { OPTIONAL = false, MANDATORY = true };
  enum class HasValue : bool { IMPLICIT_VALUE = false, EXPLICIT_VALUE = true };

  void varFlag(const std::string& name, std::function<void(const std::string&)> parse, const std::string& description,
               Opt opt, HasValue hasValue);
  void boolFlag(bool* value, const std::string& name, bool defaultValue, const std::string& description,
                Opt opt = Opt::OPTIONAL);
  void stringFlag(std::string* value, const std::string& name, const std::string& defaultValue,
                  const std::string& description, Opt opt = Opt::OPTIONAL);

  size_t parse(int argc, char* argv[]);
  size_t parse(const std::vector<std::string>& args);
  void printUsage(std::ostream& out);

 private:
  struct FlagSpec {
    std::string name;
    std::function<void(const std::string&)> parse;
    std::string description;
    Opt opt;
    HasValue hasValue;

    static bool less(const FlagSpec& l, const std::string& r);
  };

  std::string programName_, shortUsage_;
  std::vector<FlagSpec> flags_;  // sorted by name
};

}  // namespace codeswitch

#endif
