// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef block_h
#define block_h

#include "bitmap.h"

namespace codeswitch {
namespace internal {

#define BLOCK_TYPE_LIST(V) \
  V(Meta)                  \
  V(Free)

#define ENUM_BLOCK_TYPE(Name) k##Name##BlockType,
enum BlockType {
  // clang-format off
  BLOCK_TYPE_LIST(ENUM_BLOCK_TYPE)
  kLastBlockType
  // clang-format on
};
#undef ENUM_BLOCK_TYPE

class Heap;
class Meta;

/**
 * A block is an object that can be allocated on the heap.
 *
 * Blocks are not necessarily usable by the VM or the program running. For example, Free
 * blocks are used internally to represent the free list.
 *
 * The first word of each block is a pointer to a Meta, which describes the structure of
 * the block. Because blocks start with metas and not vtables, blocks may not have virtual
 * methods.
 */
class Block {
 public:
  explicit Block(Meta* meta) : meta_(meta) {}

  Meta* meta() const { return meta_; }

 private:
  Meta* meta_;
};

/**
 * Meta describes the structure of a Block.
 *
 * Metas are used by the garbage collector to traverse the graph of live objects since they
 * describe block size and location of pointers.
 *
 * Metas also store method pointers and other values that are constant across multiple
 * blocks.
 *
 * Metas are immutable after they are created.
 */
class Meta : public Block {
 public:
  static const BlockType kBlockType = kMetaBlockType;

  void* operator new(size_t, Heap* heap, length_t dataLength, word_t objectSize, word_t elementSize);
  explicit Meta(BlockType blockType) : Block(nullptr), blockType_(blockType) {}

  BlockType blockType() const { return blockType_; }

 private:
  typedef uint8_t flags_t;
  static const flags_t kHasPointers = 1 << 0;
  static const flags_t kHasElements = 1 << 1;
  static const flags_t kHasElementPointers = 1 << 2;

  length_t length_;
  BlockType blockType_ : 8;
  bool hasPointers_ : 1;
  bool hasElementPointers_ : 1;
  uint8_t lengthOffset_;
  word_t size_;
  word_t elementSize_;
  word_t data_[0];
};

/**
 * Free represents a node in the free list.
 */
class Free : public Block {
 public:
  static const BlockType kBlockType = kFreeBlockType;

  inline void* operator new(size_t, void* place, word_t size);
  explicit Free(Free* next) : Block(nullptr), next_(next) {}

  word_t size() const { return size_; }
  Free* next() const { return next_; }
  void setNext(Free* next) { next_ = next; }

 private:
  word_t size_;
  Free* next_;

  friend class Chunk;
};

void* Free::operator new(size_t, void* place, word_t size) {
  auto free = reinterpret_cast<Free*>(place);
  free->size_ = size;
  free->next_ = nullptr;
  return place;
}

/** Returns whether a block is an instance of a given block type. */
template <class T>
bool isa(const Block* block) {
  return block->meta()->blockType() == T::kBlockType;
}
}  // namespace internal
}  // namespace codeswitch

#endif
