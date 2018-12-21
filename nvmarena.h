#pragma once

#ifndef OS_WIN
#include <sys/mman.h>
#endif
#include "util/allocator.h"
#include "util/arena.h"
#include "util/core_local.h"
#include "util/mutexlock.h"

// Only generate field unused warning for padding array, or build under
// GCC 4.8.1 will fail.
#ifdef __clang__
#define ROCKSDB_FIELD_UNUSED __attribute__((__unused__))
#else
#define ROCKSDB_FIELD_UNUSED
#endif  // __clang__

namespace rocksdb {

class NVMArena : public Allocator
{
public:
  // No copying allowed
  NVMArena(const NVMArena&) = delete;
  void operator=(const NVMArena&) = delete;
  explicit NVMArena(size_t block_size = Arena::kMinBlockSize,
                    AllocTracker* tracker = nullptr, size_t huge_page_size = 0);

  char* Allocate(size_t bytes) override;
  char* AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                        Logger* logger = nullptr) override;


  char* AllocateFallback(size_t bytes, bool aligned);
  char* AllocateNewBlock(size_t block_bytes);

  // Stats for current active block.
  // For each block, we allocate aligned memory chucks from one end and
  // allocate unaligned memory chucks from the other end. Otherwise the
  // memory waste for alignment will be higher if we allocate both types of
  // memory from one direction.
  char* unaligned_alloc_ptr_ = nullptr;
  char* aligned_alloc_ptr_ = nullptr;
  // How many bytes left in currently active block?
  size_t alloc_bytes_remaining_ = 0;

  // Array of new[] allocated memory blocks
  std::vector<char*> blocks_;

  size_t blocks_memory_ = 0;
  AllocTracker* tracker_;
};

}  // namespace rocksdb
