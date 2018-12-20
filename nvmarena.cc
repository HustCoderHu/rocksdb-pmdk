#include "nvmarena.h"

namespace rocksdb {

namespace {
// If the shard block size is too large, in the worst case, every core
// allocates a block without populate it. If the shared block size is
// 1MB, 64 cores will quickly allocate 64MB, and may quickly trigger a
// flush. Cap the size instead.
const size_t kMaxShardBlockSize = size_t{128 * 1024};
}  // namespace

NVMArena::NVMArena(size_t block_size, AllocTracker *tracker, size_t huge_page_size)
    : shard_block_size_(std::min(kMaxShardBlockSize, block_size / 8)),
      shards_(),
      arena_(block_size, tracker, huge_page_size)
{

}

size_t NVMArena::ShardAllocatedAndUnused() const
{
    size_t total = 0;
    for (size_t i = 0; i < shards_.Size(); ++i) {
        total += shards_.AccessAtCore(i)->allocated_and_unused_.load(
                    std::memory_order_relaxed);
    }
    return total;
}

char *NVMArena::AllocateFallback(size_t bytes, bool aligned)
{
    if (bytes > kBlockSize / 4) {
        ++irregular_block_num;
        // Object is more than a quarter of our block size.  Allocate it separately
        // to avoid wasting too much space in leftover bytes.
        return AllocateNewBlock(bytes);
    }

    // We waste the remaining space in the current block.
    size_t size = 0;
    char* block_head = nullptr;
#ifdef MAP_HUGETLB
    if (hugetlb_size_) {
        size = hugetlb_size_;
        block_head = AllocateFromHugePage(size);
    }
#else
    size = kBlockSize;
    block_head = AllocateNewBlock(size);
#endif
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
#ifdef ROCKSDB_MALLOC_USABLE_SIZE
    allocated_size = malloc_usable_size(block);
#ifndef NDEBUG
    // It's hard to predict what malloc_usable_size() returns.
    // A callback can allow users to change the costed size.
    std::pair<size_t*, size_t*> pair(&allocated_size, &block_bytes);
    TEST_SYNC_POINT_CALLBACK("Arena::AllocateNewBlock:0", &pair);
#endif  // NDEBUG
#else
    allocated_size = block_bytes;
#endif  // ROCKSDB_MALLOC_USABLE_SIZE
    blocks_memory_ += allocated_size;
    if (tracker_ != nullptr) {
        tracker_->Allocate(allocated_size);
    }
    blocks_.back() = block;
    return block;
}

void NVMArena::Fixup() {
    arena_allocated_and_unused_.store(arena_.AllocatedAndUnused(),
                                      std::memory_order_relaxed);
    memory_allocated_bytes_.store(arena_.MemoryAllocatedBytes(),
                                  std::memory_order_relaxed);
    irregular_block_num_.store(arena_.IrregularBlockNum(),
                               std::memory_order_relaxed);
}

char *AllocateAlligned(size_t bytes, size_t huge_page_size, Logger *logger)
{
    size_t rounded_up = ((bytes - 1) | (sizeof(void*) - 1)) + 1;
    assert(rounded_up >= bytes && rounded_up < bytes + sizeof(void*) &&
           (rounded_up % sizeof(void*)) == 0);

    const int align = sizeof(void*);
    size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align-1);
    size_t slop = (current_mod == 0 ? 0 : align - current_mod);
    size_t needed = bytes + slop;
    char* result;
    if (needed <= alloc_bytes_remaining_) {
        result = alloc_ptr_ + slop;
        alloc_ptr_ += needed;
        alloc_bytes_remaining_ -= needed;
    } else {
        // AllocateFallback always returned aligned memory
        result = AllocateFallback(bytes);
    }
    assert((reinterpret_cast<uintptr_t>(result) & (align-1)) == 0);
    return result;
}

char *rocksdb::NVMArena::Allocate(size_t bytes)
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

NVMArena::Shard *NVMArena::Repick()
{
    auto shard_and_index = shards_.AccessElementAndIndex();
#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
    // even if we are cpu 0, use a non-zero tls_cpuid so we can tell we
    // have repicked
    tls_cpuid = shard_and_index.second | shards_.Size();
#endif
    return shard_and_index.first;
}



}  // namespace rocksdb
