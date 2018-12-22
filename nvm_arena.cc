#define NVM_DEBUG

#include "nvm_arena.h"

#include <string>
#include "util/sync_point.h"
#include "libpmem.h"
#include "debug.h"

namespace rocksdb {

// MSVC complains that it is already defined since it is static in the header.
#ifndef _MSC_VER
//const size_t NVMArena::kInlineSize;
#endif

const size_t NVMArena::kMinBlockSize = 4096;
const size_t NVMArena::kMaxBlockSize = 2u << 30;
static const int kAlignUnit = alignof(max_align_t);
int NVMArena::map_file_idx = 0;

NVMArena::NVMArena(size_t block_size,
                   AllocTracker *tracker, size_t huge_page_size)
    : block_sz(OptimizeBlockSize(block_size<<3))
    , kBlockSize(OptimizeBlockSize(block_size<<3))
//    , file_name_(filename)
//    , tracker_(tracker)
{
//    assert(kBlockSize >= kMinBlockSize && kBlockSize <= kMaxBlockSize &&
//           kBlockSize % kAlignUnit == 0);
//    TEST_SYNC_POINT_CALLBACK("Arena::Arena:0", const_cast<size_t*>(&kBlockSize));
    std::string str("/mnt/pmem/");
    str += std::to_string(map_file_idx++) + ".nvm_arena";

    DBG_PRINT("inparam block_size: %zu", block_size);
    DBG_PRINT("block_sz: %zu", block_sz);

    block_addr = static_cast<char*>(pmem_map_file(str.c_str(),
                                                  block_sz, PMEM_FILE_CREATE|PMEM_FILE_EXCL,
                                                  0666, &mapped_len_, &is_pmem_));
    if(block_addr == NULL){
        DBG_PRINT("map error");
        exit(-1);
    }
//    capacity_ = size;
//    now_ = aligned_alloc_ptr_;

    aligned_alloc_ptr_ = block_addr;
    alloc_bytes_remaining_ = block_sz;
    blocks_memory_ += alloc_bytes_remaining_;
    unaligned_alloc_ptr_ = aligned_alloc_ptr_ + alloc_bytes_remaining_;

    (void)huge_page_size;
//    if (tracker_ != nullptr) {
//        tracker_->Allocate(kInlineSize);
    //    }
}

NVMArena::~NVMArena()
{
//    pmem_unmap(block_addr, block_sz);
}

char *NVMArena::Allocate(size_t bytes)
{
    // The semantics of what to return are a bit messy if we allow
    // 0-byte allocations, so we disallow them here (we don't need
    // them for our internal use).
    assert(bytes > 0);
    if (bytes <= alloc_bytes_remaining_) {
        unaligned_alloc_ptr_ -= bytes;
        alloc_bytes_remaining_ -= bytes;
        return unaligned_alloc_ptr_;
    }
    return AllocateFallback(bytes, false /* unaligned */);
}

char *NVMArena::AllocateAligned(size_t bytes, size_t huge_page_size, Logger *logger)
{
    assert((kAlignUnit & (kAlignUnit - 1)) == 0);
    // Pointer size should be a power of 2
    (void)huge_page_size;
    (void)logger;

    size_t current_mod =
            reinterpret_cast<uintptr_t>(aligned_alloc_ptr_) & (kAlignUnit - 1);
    size_t slop = (current_mod == 0 ? 0 : kAlignUnit - current_mod);
    size_t needed = bytes + slop;
    char* result;
//    DBG_PRINT("bytes: %zu", bytes);
//    DBG_PRINT("needed: %zu", needed);
//    DBG_PRINT("alloc_bytes_remaining_: %zu", alloc_bytes_remaining_);
    if (needed <= alloc_bytes_remaining_) {
        result = aligned_alloc_ptr_ + slop;
        aligned_alloc_ptr_ += needed;
        alloc_bytes_remaining_ -= needed;
    } else {
        // AllocateFallback always returns aligned memory
        result = AllocateFallback(bytes, true /* aligned */);
    }
    assert((reinterpret_cast<uintptr_t>(result) & (kAlignUnit - 1)) == 0);
    return result;
}

char *NVMArena::AllocateFallback(size_t bytes, bool aligned)
{
    // error
    DBG_PRINT("error - pmdk no new !!!");
//    return nullptr;
    exit(-1);

    if (bytes > kBlockSize / 4) {
        ++irregular_block_num;
        // Object is more than a quarter of our block size.  Allocate it separately
        // to avoid wasting too much space in leftover bytes.
        return AllocateNewBlock(bytes);
    }

    // We waste the remaining space in the current block.
    size_t size = 0;
    char* block_head = nullptr;
    size = kBlockSize;
    block_head = AllocateNewBlock(size);
    alloc_bytes_remaining_ = size - bytes;

    if (aligned) {
        aligned_alloc_ptr_ = block_head + bytes;
        unaligned_alloc_ptr_ = block_head + size;
        return block_head;
    } else {
        aligned_alloc_ptr_ = block_head;
        unaligned_alloc_ptr_ = block_head + size - bytes;
        return unaligned_alloc_ptr_;
    }
}

// from rocksdb arena.cc
char *NVMArena::AllocateNewBlock(size_t block_bytes)
{
    // Reserve space in `blocks_` before allocating memory via new.
    // Use `emplace_back()` instead of `reserve()` to let std::vector manage its
    // own memory and do fewer reallocations.
    //
    // - If `emplace_back` throws, no memory leaks because we haven't called `new`
    //   yet.
    // - If `new` throws, no memory leaks because the vector will be cleaned up
    //   via RAII.
    blocks_.emplace_back(nullptr);

    char* block = new char[block_bytes];
    size_t allocated_size;

    allocated_size = block_bytes;

    blocks_memory_ += allocated_size;
//    if (tracker_ != nullptr) {
//        tracker_->Allocate(allocated_size);
//    }
    blocks_.back() = block;
    return block;
}

}  // namespace rocksdb
