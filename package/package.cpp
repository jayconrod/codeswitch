// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "package.h"

#include <array>
#include <filesystem>
#include "common/common.h"
#include "common/file.h"
#include "common/str.h"
#include "memory/handle.h"
#include "platform/platform.h"
#include "type.h"

#include <iostream>

namespace filesystem = std::filesystem;

namespace codeswitch {

Function* Package::functionByIndex(size_t index) {
  std::lock_guard<std::mutex> lock(mu_);
  return functionByIndexLocked(index);
}

Function* Package::functionByName(const String& name) {
  std::lock_guard<std::mutex> lock(mu_);
  return functionByNameLocked(name);
}

Handle<Package> Package::readFromFile(const filesystem::path& filename) {
  MappedFile file(filename, MappedFile::READ);
  if (file.size < kFileHeaderSize) {
    throw FileError(filename, "file is too small to contain file header");
  }

  auto p = file.data;
  FileHeader fh;
  readFileHeader(&p, &fh);
  if (fh.magic != kMagic) {
    throw FileError(filename, "unknown package file format");
  }
  if (fh.version != 0) {
    throw FileError(filename, "unknown version of codeswitch package format");
  }
  if (fh.wordSize != 8) {
    throw FileError(filename, "unsupported word size");
  }

  uintptr_t endOfHeaders = (p - file.data) + fh.sectionCount * kSectionHeaderSize;
  if (endOfHeaders > file.size) {
    throw FileError(filename, "file is too small to contain section headers");
  }
  SectionHeader functionSection{}, typeSection{}, stringSection{};
  auto prevEnd = endOfHeaders;
  for (int i = 0; i < fh.sectionCount; i++) {
    SectionHeader sh;
    readSectionHeader(&p, &sh);
    auto dataOffset = static_cast<uint64_t>(sh.entryCount) * static_cast<uint64_t>(sh.entrySize);
    if (dataOffset > sh.size) {
      throw FileError(filename, strprintf("in section %d, data offset is out of bounds", i));
    }
    if (sh.offset != prevEnd) {
      throw FileError(filename, strprintf("section %d is not immediately after previous section", i));
    }
    if (addWouldOverflow(prevEnd, sh.size)) {
      throw FileError(filename, strprintf("overflow when computing end offset of section %d", i));
    }
    prevEnd += sh.size;
    switch (sh.kind) {
      case SectionKind::FUNCTION:
        if (functionSection.offset > 0) {
          throw FileError(filename, "duplicate function section");
        }
        if (sh.entrySize < kFunctionEntrySize) {
          throw FileError(filename, "function section entries are too small");
        }
        functionSection = sh;
        break;
      case SectionKind::TYPE:
        if (typeSection.offset > 0) {
          throw FileError(filename, "duplicate type section");
        }
        typeSection = sh;
        break;
      case SectionKind::STRING:
        if (stringSection.offset > 0) {
          throw FileError(filename, "duplicate string section");
        }
        if (sh.entrySize < kStringEntrySize) {
          throw FileError(filename, "string section entries are too small");
        }
        stringSection = sh;
        break;
      default:
        // Ignore sections of unknown type.
        break;
    }
  }
  if (prevEnd != file.size) {
    throw FileError(filename, "unexpected space at end of file");
  }

  auto package = handle(new (heap->allocate(sizeof(Package)))
                            Package(std::move(file), functionSection, typeSection, stringSection));
  package->functions_.resize(functionSection.entryCount);
  package->types_.resize(typeSection.entryCount);
  package->strings_.resize(stringSection.entryCount);
  return package;
}

void Package::writeToFile(const filesystem::path& filename) {
  std::lock_guard lock(mu_);
  populateLocked();

  // Gather all strings referenced by the package. We'll deduplicate them
  // to save space.
  auto stringIndex = handle(Map<String, uint32_t, HashString>::make());
  std::vector<StringEntry> stringEntries;
  std::vector<uint8_t> stringData;
  auto visitString = [&stringIndex, &stringEntries, &stringData](const String& s) {
    if (stringIndex->has(s)) {
      return;
    }
    stringIndex->set(s, stringEntries.size());
    stringEntries.emplace_back(StringEntry{.offset = stringData.size(), .size = s.length()});
    stringData.insert(stringData.end(), s.begin(), s.end());
  };
  for (auto& function : functions_) {
    visitString(function->name);
  }

  // Gather all types referenced by the package. These are not deduplicated,
  // though that might save some space. Each function just references the
  // beginning offset of its input and output type list, and we read that
  // many types. If they were deduplicated, they wouldn't be together.
  struct FunctionTypeLocation {
    uint64_t paramTypeOffset, returnTypeOffset;
  };
  std::vector<FunctionTypeLocation> typeOffsets(functions_.length());
  std::vector<uint8_t> typeData;
  for (size_t i = 0, n = functions_.length(); i < n; i++) {
    auto& f = functions_[i];
    typeOffsets[i].paramTypeOffset = typeData.size();
    for (auto& t : f->paramTypes) {
      writeType(&typeData, t.get());
    }
    typeOffsets[i].returnTypeOffset = typeData.size();
    for (auto& t : f->returnTypes) {
      writeType(&typeData, t.get());
    }
  }

  // Build an offset list of instructions and safepoints referenced by
  // functions. This is simpler than the above because there's no deduplication.
  std::vector<uint64_t> instOffsets;
  instOffsets.reserve(functions_.length());
  std::vector<uint64_t> safepointOffsets;
  safepointOffsets.reserve(functions_.length());
  uint64_t lastFunctionDataOffset = 0;
  for (auto& f : functions_) {
    instOffsets.push_back(lastFunctionDataOffset);
    lastFunctionDataOffset += f->insts.length();
    safepointOffsets.push_back(lastFunctionDataOffset);
    lastFunctionDataOffset += f->safepoints.data().length();
  }

  // Assemble headers, figure out where everything is and how big it will be.
  auto fileHeader = FileHeader{
      .magic = kMagic,
      .version = 0,
      .wordSize = sizeof(uintptr_t),
      .sectionCount = 3,
  };
  auto functionSection = SectionHeader{
      .kind = SectionKind::FUNCTION,
      .offset = kFileHeaderSize + 3 * kSectionHeaderSize,
      .size = functions_.length() * kFunctionEntrySize + lastFunctionDataOffset,
      .entryCount = narrow<uint32_t>(functions_.length()),
      .entrySize = kFunctionEntrySize,
  };
  auto typeSection = SectionHeader{
      .kind = SectionKind::TYPE,
      .offset = functionSection.offset + functionSection.size,
      .size = typeData.size(),
      .entryCount = 0,
      .entrySize = 0,
  };
  auto stringSection = SectionHeader{
      .kind = SectionKind::STRING,
      .offset = typeSection.offset + typeSection.size,
      .size = stringEntries.size() * kStringEntrySize + stringData.size(),
      .entryCount = narrow<uint32_t>(stringEntries.size()),
      .entrySize = kStringEntrySize,
  };
  auto fileSize = stringSection.offset + stringSection.size;
  std::array<SectionHeader*, 3> sections{&functionSection, &typeSection, &stringSection};

  // Write file header.
  MappedFile file(filename, fileSize, 0666);
  auto p = file.data;
  writeFileHeader(&p, fileHeader);

  // Write section headers.
  for (auto sh : sections) {
    writeSectionHeader(&p, *sh);
  }

  // Write function section.
  ASSERT(static_cast<uint64_t>(p - file.data) == functionSection.offset);
  for (size_t i = 0, n = functions_.length(); i < n; i++) {
    auto& f = functions_[i];
    FunctionEntry fe{
        .nameIndex = stringIndex->get(f->name),
        .paramTypeOffset = typeOffsets[i].paramTypeOffset,
        .paramTypeCount = narrow<uint32_t>(f->paramTypes.length()),
        .returnTypeOffset = typeOffsets[i].returnTypeOffset,
        .returnTypeCount = narrow<uint32_t>(f->returnTypes.length()),
        .instOffset = instOffsets[i],
        .instSize = narrow<uint32_t>(f->insts.length()),
        .safepointOffset = safepointOffsets[i],
        .safepointCount = f->safepoints.length(),
        .frameSize = static_cast<uint16_t>(f->safepoints.frameSize()),
    };
    writeFunctionEntry(&p, fe);
  }
  for (auto& f : functions_) {
    p = std::copy(reinterpret_cast<uint8_t*>(f->insts.begin()), reinterpret_cast<uint8_t*>(f->insts.end()), p);
    p = std::copy(f->safepoints.data().begin(), f->safepoints.data().end(), p);
  }

  // Write type section.
  ASSERT(static_cast<uint64_t>(p - file.data) == typeSection.offset);
  p = std::copy(typeData.begin(), typeData.end(), p);

  // Write string section.
  ASSERT(static_cast<uint64_t>(p - file.data) == stringSection.offset);
  for (auto& e : stringEntries) {
    writeStringEntry(&p, e);
  }
  p = std::copy(stringData.begin(), stringData.end(), p);
}

/**
 * Checks that a package in memory satisfies all invariants expected.
 * validate completely loads the package from a binary file (if there is one),
 * so it's not normally called when loading a package into memory. However,
 * packages should be validated at least once (for example, at install time)
 * before being interpreted.
 */
void Package::validate() {
  std::lock_guard lock(mu_);
  populateLocked();
  auto p = handle(this);

  try {
    for (auto& f : functions_) {
      f->validate(p);
    }
  } catch (ValidateError& err) {
    err.filename = filename_;
    throw err;
  }
}

Function* Package::functionByIndexLocked(size_t index) {
  if (functions_[index]) {
    return functions_[index].get();
  }
  auto p = file_.data + functionSection_.offset + index * functionSection_.entrySize;
  FunctionEntry entry;
  readFunctionEntry(&p, &entry);

  auto function = new (heap->allocate(sizeof(Function))) Function;
  functions_[index] = function;
  function->name = stringByIndexLocked(entry.nameIndex);
  readTypeList(&function->paramTypes, entry.paramTypeCount, entry.paramTypeOffset);
  readTypeList(&function->returnTypes, entry.returnTypeCount, entry.returnTypeOffset);
  auto instBegin = reinterpret_cast<Inst*>(file_.data + functionSection_.offset +
                                           functionSection_.entryCount * functionSection_.entrySize + entry.instOffset);
  if (addWouldOverflow(reinterpret_cast<uintptr_t>(instBegin), static_cast<uintptr_t>(entry.instSize))) {
    throw errorstr(filename_, ": for function ", index, ", overflow computing end of instructions");
  }
  auto instEnd = instBegin + entry.instSize;
  auto functionSectionEnd = file_.data + functionSection_.offset + functionSection_.size;
  if (instEnd > reinterpret_cast<Inst*>(functionSectionEnd)) {
    throw errorstr(filename_, ": for function ", index, ", end of instructions outside function section");
  }
  function->insts.resize(entry.instSize);
  std::copy(instBegin, instBegin + entry.instSize, function->insts.begin());
  auto safepointsBegin = file_.data + functionSection_.offset + functionSection_.entryCount * functionSection_.entrySize + entry.safepointOffset;
  auto safepointsSize = static_cast<uintptr_t>(Safepoints::bytesPerEntry(entry.frameSize)) * entry.safepointCount;
  if (addWouldOverflow(reinterpret_cast<uintptr_t>(safepointsBegin), safepointsSize)) {
    throw errorstr(filename_, ": for function ", index, ", overflow computing end of safepoints");
  }
  auto safepointsEnd = safepointsBegin + safepointsSize;
  if (safepointsEnd > functionSectionEnd) {
    throw errorstr(filename_, ": for function ", index, ", end of safepoints outside function section");
  }
  auto safepointsData = handle(new (heap->allocate(sizeof(BoundArray<uint8_t>))) BoundArray<uint8_t>);
  safepointsData->init(Array<uint8_t>::make(safepointsSize), safepointsSize);
  std::copy(safepointsBegin, safepointsEnd, safepointsData->begin());
  function->safepoints.init(entry.frameSize, **safepointsData);

  return function;
}

Function* Package::functionByNameLocked(const String& name) {
  if (functions_.empty() || !functionsByName_.empty()) {
    return functionsByName_.get(name).get();
  }

  for (size_t i = 0, n = functions_.length(); i < n; i++) {
    auto function = functionByIndex(i);
    functionsByName_.set(function->name, function);
  }
  return functionsByName_.get(name).get();
}

String& Package::stringByIndexLocked(size_t index) {
  if (!strings_[index].isNull()) {
    return strings_[index];
  }

  auto p = file_.data + stringSection_.offset + index * stringSection_.entrySize;
  StringEntry entry;
  readStringEntry(&p, &entry);
  auto dataBegin =
      file_.data + stringSection_.offset + stringSection_.entryCount * stringSection_.entrySize + entry.offset;
  if (addWouldOverflow(reinterpret_cast<uintptr_t>(dataBegin), static_cast<uintptr_t>(entry.size))) {
    throw errorstr(filename_, ": for string ", index, ", overflow computing end of string");
  }
  auto dataEnd = dataBegin + entry.size;
  auto stringSectionEnd = file_.data + stringSection_.offset + stringSection_.size;
  if (dataEnd > stringSectionEnd) {
    throw errorstr(filename_, ": for function ", index, ", end of string outside string section");
  }
  auto data = Array<uint8_t>::make(entry.size);
  std::copy(dataBegin, dataEnd, data->begin());
  strings_[index].init(reinterpret_cast<Array<const uint8_t>*>(data), entry.size);
  return strings_[index];
}

void Package::readTypeList(List<Ptr<Type>>* types, uint32_t count, uint64_t offset) {
  auto p = file_.data + typeSection_.offset + typeSection_.entryCount * typeSection_.entrySize + offset;
  auto end = file_.data + typeSection_.offset + typeSection_.size;
  types->resize(count);
  for (size_t i = 0; i < count; i++) {
    types->at(i) = readType(&p, end);
  }
}

Type* Package::readType(uint8_t** p, uint8_t* end) {
  if (*p >= end) {
    throw errorstr(file_.filename, ": type outside of type section");
  }
  auto kind = static_cast<Type::Kind>(readBin<uint8_t>(p));
  switch (kind) {
    case Type::UNIT:
    case Type::BOOL:
    case Type::INT64:
      return Type::make(kind);
    default:
      throw errorstr(file_.filename, ": unknown type kind");
  }
}

void Package::writeType(std::vector<uint8_t>* data, const Type* type) {
  data->push_back(static_cast<uint8_t>(type->kind()));
}

void Package::populateLocked() {
  for (size_t i = 0, n = functions_.length(); i < n; i++) {
    functionByIndexLocked(i);
  }
}

void Package::readFileHeader(uint8_t** p, FileHeader* fh) {
  *fh = FileHeader{};
  readBin(p, &fh->magic);
  readBin(p, &fh->version);
  readBin(p, &fh->wordSize);
  readBin(p, &fh->sectionCount);
}

void Package::writeFileHeader(uint8_t** p, FileHeader fh) {
  writeBin(p, fh.magic);
  writeBin(p, fh.version);
  writeBin(p, fh.wordSize);
  writeBin(p, fh.sectionCount);
}

void Package::readSectionHeader(uint8_t** p, SectionHeader* sh) {
  *sh = SectionHeader{};
  readBin(p, &sh->kind);
  readBin(p, &sh->offset);
  readBin(p, &sh->size);
  readBin(p, &sh->entryCount);
  readBin(p, &sh->entrySize);
}

void Package::writeSectionHeader(uint8_t** p, SectionHeader sh) {
  writeBin(p, static_cast<uint32_t>(sh.kind));
  writeBin(p, sh.offset);
  writeBin(p, sh.size);
  writeBin(p, sh.entryCount);
  writeBin(p, sh.entrySize);
}

void Package::readFunctionEntry(uint8_t** p, FunctionEntry* e) {
  readBin(p, &e->nameIndex);
  readBin(p, &e->paramTypeOffset);
  readBin(p, &e->paramTypeCount);
  readBin(p, &e->returnTypeOffset);
  readBin(p, &e->returnTypeCount);
  readBin(p, &e->instOffset);
  readBin(p, &e->instSize);
  readBin(p, &e->safepointOffset);
  readBin(p, &e->safepointCount);
  readBin(p, &e->frameSize);
}

void Package::writeFunctionEntry(uint8_t** p, FunctionEntry e) {
  writeBin(p, e.nameIndex);
  writeBin(p, e.paramTypeOffset);
  writeBin(p, e.paramTypeCount);
  writeBin(p, e.returnTypeOffset);
  writeBin(p, e.returnTypeCount);
  writeBin(p, e.instOffset);
  writeBin(p, e.instSize);
  writeBin(p, e.safepointOffset);
  writeBin(p, e.safepointCount);
  writeBin(p, e.frameSize);
}

void Package::readStringEntry(uint8_t** p, StringEntry* e) {
  readBin(p, &e->offset);
  readBin(p, &e->size);
}

void Package::writeStringEntry(uint8_t** p, StringEntry e) {
  writeBin(p, e.offset);
  writeBin(p, e.size);
}

ValidateError::ValidateError(const std::filesystem::path& filename, const std::string& defName,
                             const std::string& message) :
    Error(""), filename(filename), defName(defName), message(message) {
  std::stringstream buf;
  if (!filename.empty()) {
    buf << filename << ": ";
  }
  if (!defName.empty()) {
    buf << defName << ": ";
  }
  buf << message;
  what_ = buf.str();
}

const char* ValidateError::what() const noexcept {
  return what_.c_str();
}

}  // namespace codeswitch
