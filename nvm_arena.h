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
    explicit NVMArena(std::string &filename, size_t block_size = Arena::kMinBlockSize,
                      AllocTracker* tracker = nullptr, size_t huge_page_size = 0);
    ~NVMArena();

    char* Allocate(size_t bytes) override;
    char* AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                          Logger* logger = nullptr) override;


    char* AllocateFallback(size_t bytes, bool aligned);
    char* AllocateNewBlock(size_t block_bytes);

    // check and adjust the block_size so that the return value is
    //  1. in the range of [kMinBlockSize, kMaxBlockSize].
    //  2. the multiple of align unit.
    extern size_t OptimizeBlockSize(size_t block_size);

    static const size_t kInlineSize = 2048;
    static const size_t kMinBlockSize;
    static const size_t kMaxBlockSize;

    std::string file_name_;
    std::vector<std::string> file_vec;
    size_t irregular_block_num = 0;

    char *block_addr;
    size_t block_sz;
    // Stats for current active block.
    // For each block, we allocate aligned memory chucks from one end and
    // allocate unaligned memory chucks from the other end. Otherwise the
    // memory waste for alignment will be higher if we allocate both types of
    // memory from one direction.
    // 对齐的内存从左往右
    // 没对齐的从右往左
    char* unaligned_alloc_ptr_ = nullptr;
    char* aligned_alloc_ptr_ = nullptr;
    // How many bytes left in currently active block?
    size_t alloc_bytes_remaining_ = 0;

    const size_t kBlockSize;
    // Array of new[] allocated memory blocks
    std::vector<char*> blocks_;

    size_t blocks_memory_ = 0;
//    AllocTracker* tracker_;

    size_t mapped_len_;
    int is_pmem_;
};

}  // namespace rocksdb
