#ifndef PERSISTENT_RANGE_MEM_H
#define PERSISTENT_RANGE_MEM_H

#include <list>

#include <db/db_impl.h>
//#include <rocksdb/slice.h>
//#include <rocksdb/iterator.h>
//#include <table/merging_iterator.h>

#include <libpmemobj.h>
#include <libpmemobj++/pool.hpp>

#include <chunkblk.h>
#include <persistent_chunk.h>
#include <pmem_hash_map.h>
#include <nvm_cache_options.h>

namespace rocksdb {

#define CHUNK_BLOOM_FILTER_LEN 8
using std::list;

//using namespace pmem;
//using namespace pmem::obj;
using pmem::obj::pool_base;
using pmem::obj::pool;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;
using pmem::obj::transaction;

struct Usage{
  uint64_t chunk_num;
  uint64_t range_size;
  Slice start,end;

};

struct freqUpdateInfo {
public:
  explicit freqUpdateInfo(uint64_t max_size)
    : MAX_CHUNK_SIZE(max_size) {

  }
  void update(pool_base &pop, uint64_t total_size, const Slice &real_start, const Slice &real_end) {
    transaction::run(pop, [&] {
      key_range_ = make_persistent<char[]>(real_start.size() + real_end.size() + 2 * sizeof(uint64_t));
      char *buf = &key_range_[0];
      EncodeFixed64(buf, real_start.size());
      memcpy(buf + sizeof(uint64_t), real_start.data(), real_start.size());
      buf += sizeof(uint64_t) + real_start.size();

      EncodeFixed64(buf, real_end.size());
      memcpy(buf + sizeof(uint64_t), real_end.data(), real_end.size());

      chunk_num_ = chunk_num_ + 1;
      seq_num_ = seq_num_ + 1;
    });
  }

  // 实际的range
  // TODO 初始值怎么定
  Slice real_start_;
  Slice real_end_;
  size_t chunk_num;
};

class FixedRangeTab
{
  using p_range::p_node;
  //  struct chunk_blk {
  //    unsigned char bloom_filter[CHUNK_BLOOM_FILTER_LEN];
  //    size_t size;
  //    char data[];
  //  };

public:
  FixedRangeTab(pool_base& pop, p_node pmap_node, FixedRangeBasedOptions *options);
  ~FixedRangeTab() = default;

public:
  // 返回当前RangeMemtable中所有chunk的有序序列
  // 基于MergeIterator
  // 参考 DBImpl::NewInternalIterator
  InternalIterator* NewInternalIterator(ColumnFamilyData* cfd, Arena* arena);
  Status Get(const InternalKeyComparator &internal_comparator, const Slice &key,
             std::string *value);

  void buildBlkList();

  // 返回当前RangeMemtable是否正在被compact
  bool IsCompactWorking() { return in_compaction_; }

  // 返回当前RangeMemtable的Global Bloom Filter
  //    char* GetBloomFilter();
  // 设置compaction状态
  void SetCompactionWorking(bool working){in_compaction_ = working;}

  // 将新的chunk数据添加到RangeMemtable
  void Append(const char *bloom_data, const Slice& chunk_data,
              const Slice& new_start, const Slice& new_end);

  // 更新当前RangeMemtable的Global Bloom Filter
  //    void SetBloomFilter(char* bloom_data);

  // 返回当前RangeMem的真实key range（stat里面记录）
  void GetRealRange(Slice& real_start, Slice& real_end);

  // 更新当前RangeMemtable的状态
  //  void UpdateStat(const Slice& new_start, const Slice& new_end);

  // 判断是否需要compact，如果需要则将一个RangeMemid加入Compact队列
  void MaybeScheduleCompact();
  
  Usage RangeUsage();

  // 释放当前RangeMemtable的所有chunk以及占用的空间
  void Release();

  // 重置Stat数据以及bloom filter
  void CleanUp();

  FixedRangeTab(const FixedRangeTab&) = delete;
  FixedRangeTab& operator=(const FixedRangeTab&) = delete;

  //private:
  uint64_t max_chunk_num_to_flush() const {
    // TODO: set a max chunk num
    return 100;
  }
  Status searchInChunk(PersistentChunkIterator *iter, InternalKeyComparator &icmp,
                       const Slice &key, std::string *value);

  Slice GetKVData(char *raw, uint64_t item_off);
  void CheckAndUpdateKeyRange(InternalKeyComparator* icmp, const Slice& new_start,
                              const Slice& new_end);

  // persistent info
  p_node pmap_node;
  pool_base& pop_;
//  pool<p_range::pmem_hash_map> *pop_;
  persistent_ptr<freqUpdateInfo> range_info_;

  // volatile info
  const FixedRangeBasedOptions *interal_options_;
  std::vector<uint64_t> chunk_offset_;
  char *raw_;
  bool in_compaction_;


  vector<ChunkBlk> blklist;
  //    list<size_t> psttChunkList;
  //    char *g_bloom_data;

  //  RangeStat stat;
  freqUpdateInfo info;

  //  unsigned int memid;
  //  std::string file_path;

  // 每个 chunk block 的偏移
  //    vector<size_t> chunkBlkOffset;

  //    persistent_ptr<char[]> buf_;

  //  size_t chunk_sum_size;
  //  const size_t MAX_CHUNK_SUM_SIZE;
};

} // namespace rocksdb

#endif // PERSISTENT_RANGE_MEM_H
