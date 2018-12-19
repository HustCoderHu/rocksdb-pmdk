#pragma once

#ifndef OS_WIN
#include <sys/mman.h>
#endif
#include "util/allocator.h"

namespace rocksdb {
class NVMArena : public Allocator
{
public:
    // No copying allowed
    NVMArena(const NVMArena&) = delete;
    void operator=(const NVMArena&) = delete;
    explicit NVMArena(size_t block_size = kMinBlockSize,
                      AllocTracker* tracker = nullptr, size_t huge_page_size = 0);
    ~NVMArena();

    char* Allocate(size_t bytes) override;
    char* AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                          Logger* logger = nullptr) override;

    // Stats for current active block.
    // For each block, we allocate aligned memory chucks from one end and
    // allocate unaligned memory chucks from the other end. Otherwise the
    // memory waste for alignment will be higher if we allocate both types of
    // memory from one direction.
    char* unaligned_alloc_ptr_ = nullptr;
    char* aligned_alloc_ptr_ = nullptr;
    // How many bytes left in currently active block?
    size_t alloc_bytes_remaining_ = 0;
};

inline char* NVMArena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    unaligned_alloc_ptr_ -= bytes;
    alloc_bytes_remaining_ -= bytes;
    return unaligned_alloc_ptr_;
  }
  char* AllocateFallback(size_t bytes, bool aligned);
  return AllocateFallback(bytes, false /* unaligned */);
}

}  // namespace rocksdb
