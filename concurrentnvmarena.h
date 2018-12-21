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
class ConcurrentNVMArena : public Allocator
{
public:
    // No copying allowed
    ConcurrentNVMArena(const ConcurrentNVMArena&) = delete;
    void operator=(const ConcurrentNVMArena&) = delete;
    explicit ConcurrentNVMArena(size_t block_size = Arena::kMinBlockSize,
                      AllocTracker* tracker = nullptr, size_t huge_page_size = 0);
    ~ConcurrentNVMArena();

    char* Allocate(size_t bytes) override;
    char* AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                          Logger* logger = nullptr) override;

    size_t MemoryAllocatedBytes() const {
        return memory_allocated_bytes_.load(std::memory_order_relaxed);
    }
    size_t AllocatedAndUnused() const {
        return arena_allocated_and_unused_.load(std::memory_order_relaxed) +
                ShardAllocatedAndUnused();
    }
    size_t IrregularBlockNum() const {
        return irregular_block_num_.load(std::memory_order_relaxed);
    }
    char *AllocateNVMBlock(size_t block_bytes);
    size_t BlockSize() const override { return kBlockSize; }
    Shard* Repick();

    size_t ShardAllocatedAndUnused() const;
    template <typename Func>
    char* AllocateImpl(size_t bytes, bool force_arena, const Func& func) {
        size_t cpu;
        // Go directly to the arena if the allocation is too large, or if
        // we've never needed to Repick() and the arena mutex is available
        // with no waiting.  This keeps the fragmentation penalty of
        // concurrency zero unless it might actually confer an advantage.
        std::unique_lock<SpinMutex> arena_lock(arena_mutex_, std::defer_lock);
        if (bytes > shard_block_size_ / 4 || force_arena ||
                ((cpu = tls_cpuid) == 0 &&
                 !shards_.AccessAtCore(0)->allocated_and_unused_.load(
                     std::memory_order_relaxed) &&
                 arena_lock.try_lock())) {
            if (!arena_lock.owns_lock()) {
                arena_lock.lock();
            }
            auto rv = func();
            Fixup();
            return rv;
        }

        // pick a shard from which to allocate
        Shard* s = shards_.AccessAtCore(cpu & (shards_.Size() - 1));
        if (!s->mutex.try_lock()) {
            s = Repick();
            s->mutex.lock();
        }
        std::unique_lock<SpinMutex> lock(s->mutex, std::adopt_lock);

        size_t avail = s->allocated_and_unused_.load(std::memory_order_relaxed);
        if (avail < bytes) {
            // reload
            std::lock_guard<SpinMutex> reload_lock(arena_mutex_);
            // If the arena's current block is within a factor of 2 of the right
            // size, we adjust our request to avoid arena waste.
            auto exact = arena_allocated_and_unused_.load(std::memory_order_relaxed);
            assert(exact == arena_.AllocatedAndUnused());
            if (exact >= bytes && arena_.IsInInlineBlock()) {
                // If we haven't exhausted arena's inline block yet, allocate from arena
                // directly. This ensures that we'll do the first few small allocations
                // without allocating any blocks.
                // In particular this prevents empty memtables from using
                // disproportionately large amount of memory: a memtable allocates on
                // the order of 1 KB of memory when created; we wouldn't want to
                // allocate a full arena block (typically a few megabytes) for that,
                // especially if there are thousands of empty memtables.
                auto rv = func();
                Fixup();
                return rv;
            }
            avail = exact >= shard_block_size_ / 2 && exact < shard_block_size_ * 2
                    ? exact
                    : shard_block_size_;
            s->free_begin_ = arena_.AllocateAligned(avail);
            Fixup();
        }
        s->allocated_and_unused_.store(avail - bytes, std::memory_order_relaxed);
        char* rv;
        if ((bytes % sizeof(void*)) == 0) {
            // aligned allocation from the beginning
            rv = s->free_begin_;
            s->free_begin_ += bytes;
        } else {
            // unaligned from the end
            rv = s->free_begin_ + avail - bytes;
        }
        return rv;
    }

#ifdef MAP_HUGETLB
    size_t hugetlb_size_ = 0;
#endif  // MAP_HUGETLB
    char* AllocateFromHugePage(size_t bytes);
    char* AllocateFallback(size_t bytes, bool aligned);
    char* AllocateNewBlock(size_t block_bytes);

    void Fixup();

    struct Shard {
        char padding[40] ROCKSDB_FIELD_UNUSED;
        mutable SpinMutex mutex;
        char* free_begin_;
        std::atomic<size_t> allocated_and_unused_;

        Shard() : free_begin_(nullptr), allocated_and_unused_(0) {}
    };

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
    static __thread size_t tls_cpuid;
#else
    enum ZeroFirstEnum : size_t { tls_cpuid = 0 };
#endif

    char padding0[56] ROCKSDB_FIELD_UNUSED;

    size_t shard_block_size_;

    CoreLocalArray<Shard> shards_;

    Arena arena_;
    mutable SpinMutex arena_mutex_;
    std::atomic<size_t> arena_allocated_and_unused_;
    std::atomic<size_t> memory_allocated_bytes_;
    std::atomic<size_t> irregular_block_num_;

    std::string mfile;
    const size_t kBlockSize;

    size_t irregular_block_num = 0;
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



inline char* ConcurrentNVMArena::Allocate(size_t bytes);

inline char* AllocateAlligned(size_t bytes, size_t huge_page_size = 0,
                              Logger* logger = nullptr);

}  // namespace rocksdb
