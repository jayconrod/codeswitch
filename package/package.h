// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_package_h
#define package_package_h

#include "data/list.h"
#include "data/map.h"
#include "data/string.h"
#include "function.h"
#include "memory/handle.h"
#include "memory/heap.h"
#include "memory/ptr.h"
#include "platform/platform.h"

namespace codeswitch {

/**
 * Package binary format
 *
 * CodeSwitch packages can be read or written as binary files. The file format
 * is designed to allow efficient random access. A package can be loaded
 * quickly without reading and parsing every instruction in every function.
 * This is somewhat analogous to how ELF and other executable formats
 * are loaded.
 *
 * A package starts with a file header, which indicates the version, word size
 * (always 8 bytes for now), endianness (always little-endian for now), and
 * number of sections.
 *
 * A table of fixed-size section headers immediately follows the file header.
 * Each section header describes a contiguous chunk of the file that contains
 * some kind of information (functions, types, strings). Unknown section kinds
 * are ignored.
 *
 * The sections immediately follow the section headers. Sections are tightly
 * packed (no space between) and appear in the same order in which they were
 * declared by the section headers. Each section consists of a number of
 * fixed-sized entries (number and size declared by the section header),
 * followed by a blob of data that fills the remaining space.
 *
 * The function section contains metadata for each function along with its
 * bytecode instructions.
 *
 * The type section contains encoded types referenced by functions (for example,
 * parameter and return types).
 *
 * The string section contains all the strings in the package.
 */

const uint32_t kMagic = 0x50575343;  // 'CSWP' in little-endian

struct FileHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t wordSize;
  uint16_t sectionCount;
};

const word_t kFileHeaderSize = 8;

enum class SectionKind : uint32_t { FUNCTION = 1, TYPE = 2, STRING = 3 };

struct SectionHeader {
  SectionKind kind;
  uint64_t offset;
  uint64_t size;
  uint32_t entryCount;
  uint32_t entrySize;
};

const word_t kSectionHeaderSize = 28;

struct FunctionEntry {
  uint32_t nameIndex;
  uint64_t paramTypeOffset;
  uint32_t paramTypeCount;
  uint64_t returnTypeOffset;
  uint32_t returnTypeCount;
  uint64_t instOffset;
  uint32_t instSize;
  uint32_t frameSize;
};

const word_t kFunctionEntrySize = 44;

struct StringEntry {
  uint64_t offset;
  uint64_t size;
};

const word_t kStringEntrySize = 16;

class Package {
 public:
  explicit Package(List<Ptr<Function>>& functions) : functions_(functions) {}
  static Package* make(List<Ptr<Function>>& functions) {
    return new (heap.allocate(sizeof(Package))) Package(functions);
  }

  length_t functionCount() const { return functions_.length(); }
  Function* functionByIndex(length_t index);
  Function* functionByName(const String& name);

  static Handle<Package> readFromFile(const std::filesystem::path& filename);
  void writeToFile(const std::filesystem::path& filename);

 private:
  Package(MappedFile&& file, SectionHeader functionSection, SectionHeader typeSection, SectionHeader stringSection) :
      file_(std::move(file)),
      functionSection_(functionSection),
      typeSection_(typeSection),
      stringSection_(stringSection) {}

  Function* functionByIndexLocked(length_t index);
  Function* functionByNameLocked(const String& name);
  String& stringByIndexLocked(length_t index);
  void readTypeList(List<Ptr<Type>>* types, uint32_t count, uint64_t offset);
  Type* readType(uint8_t** p, uint8_t* end);
  static void writeType(std::vector<uint8_t>* data, const Type* type);

  void populateLocked();
  static void readFileHeader(uint8_t** p, FileHeader* fh);
  static void writeFileHeader(uint8_t** p, FileHeader fh);
  static void readSectionHeader(uint8_t** p, SectionHeader* sh);
  static void writeSectionHeader(uint8_t** p, SectionHeader sh);
  static void readFunctionEntry(uint8_t** p, FunctionEntry* e);
  static void writeFunctionEntry(uint8_t** p, FunctionEntry e);
  static void readStringEntry(uint8_t** p, StringEntry* e);
  static void writeStringEntry(uint8_t** p, StringEntry e);

  std::mutex mu_;

  std::filesystem::path filename_;
  List<Ptr<Function>> functions_;
  List<Ptr<Type>> types_;
  List<String> strings_;
  Map<String, Ptr<Function>, HashString> functionsByName_;

  MappedFile file_;
  SectionHeader functionSection_, typeSection_, stringSection_;
};

}  // namespace codeswitch

#endif
