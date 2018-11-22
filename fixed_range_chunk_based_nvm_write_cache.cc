#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "fixed_range_chunk_based_nvm_write_cache.h"

#include <ex_common.h> // file_exists()

namespace rocksdb {

using pmem::obj::transaction;
using pmem::obj::make_persistent;

using p_range::pmem_hash_map;
using p_range::p_node;

FixedRangeChunkBasedNVMWriteCache::FixedRangeChunkBasedNVMWriteCache(const FixedRangeBasedOptions *ioptions,
                                                                     const string& file, uint64_t pmem_size)
  :file_path(file)
  ,LAYOUT("FixedRangeChunkBasedNVMWriteCache")
  ,POOLSIZE(1 << 26) // TODO
{
  vinfo_ = new VolatileInfo(ioptions);
  //  bool justCreated = false;
  if (file_exists(file_path.c_str()) != 0) {
    pop_ = pool<PersistentInfo>::create(file_path, LAYOUT, pmem_size, CREATE_MODE_RW);
    //    justCreated = true;
  } else {
    pop_ = pool<PersistentInfo>::open(file_path, LAYOUT);
  }
  //  pmem_ptr = pop.root();

  if (!pinfo_->inited_) {
    pinfo_ = pop_.root();
    transaction::run(pop_, [&] {
      // TODO 配置
      persistent_ptr<pmem_hash_map> _range_map = make_persistent<p_range::pmem_hash_map>();
      _range_map->tabLen = ;
      _range_map->tab = make_persistent<p_node[]>(_range_map->tabLen);
      _range_map->loadFactor = 0.75f;
      _range_map->threshold = _range_map->tabLen * _range_map->loadFactor;
      _range_map->size = 0;

      pinfo_->range_map_ = _range_map;
      // TODO
      // PersistentAllocator 构造参数 ?
      pinfo_->allocator_ = make_persistent<PersistentAllocator>();

      pinfo_->inited_ = true;
    });
  }else{
    RebuildFromPersistentNode();
  }

  //  if (file_exists(file_path) != 0) {
  //    if ((pop = pmemobj_create(file_path, POBJ_LAYOUT_NAME(range_mem),
  //                              PMEMOBJ_MIN_POOL, 0666)) == NULL) {
  //      perror("failed to create pool\n");
  //      return 1;
  //    }
  //  } else {
  //    if ((pop = pmemobj_open(file_path,
  //                            POBJ_LAYOUT_NAME(range_mem))) == NULL) {
  //      perror("failed to open pool\n");
  //      return 1;
  //    }
  //  }
}

FixedRangeChunkBasedNVMWriteCache::~FixedRangeChunkBasedNVMWriteCache()
{
  delete vinfo_;
  if (pop_)
    pop_.close();
  //    pmemobj_close(pop);
}

Status FixedRangeChunkBasedNVMWriteCache::Get(const InternalKeyComparator &internal_comparator, const Slice &key, std::string *value)
{
  std::string prefix = (*vinfo_->internal_options_->prefix_extractor_)(key.data(), key.size());
  auto found_tab = vinfo_->prefix2range.find(prefix);
  if(found_tab == vinfo_->prefix2range.end()){
    // not found
    return Status::NotFound("no this range");
  }else{
    // found
    FixedRangeTab* tab = found_tab->second;
    return tab->Get(internal_comparator, key, value);
  }
}

void FixedRangeChunkBasedNVMWriteCache::AppendToRange(const InternalKeyComparator &icmp,
                                                      const char *bloom_data, const Slice &chunk_data,
                                                      const ChunkMeta &meta)
{
  /*
   * 1. 获取prefix
   * 2. 检查tab是否存在
   * 2.1 tab不存在则NewRange
   * 3. 调用tangetab的append
   * */
  FixedRangeTab* now_range = nullptr;
  auto tab_found = vinfo_->prefix2range.find(meta.prefix);
  if(tab_found == vinfo_->prefix2range.end()){
    now_range = NewRange(meta.prefix);
  }else{
    now_range = &tab_found->second;
  }
  now_range->Append(icmp, bloom_data, chunk_data, meta.cur_start, meta.cur_end);
}

FixedRangeTab* FixedRangeChunkBasedNVMWriteCache::NewRange(const std::string &prefix)
{
  // TODO
  // buf = ?  range 分配多大空间
  size_t bufSize = 1 << 26; // 64 MB
  //  uint64_t _hash;
  //  _hash = p_map->put(pop, prefix, bufSize);
  p_node node_in_pmem_map = pinfo_->range_map_->putAndGet(pop_, prefix, bufSize);
  //  return _hash;

  FixedRangeTab *range = new FixedRangeTab(pop_, node_in_pmem_map, option); // TODO
  vinfo_->prefix2range.insert({prefix, range});
  return range;
}

void FixedRangeChunkBasedNVMWriteCache::MaybeNeedCompaction()
{
  // TODO more reasonable compaction threashold
  if(pinfo_->allocator_->Remain() < pinfo_->allocator_->Capacity() * 0.75){
    uint64_t max_range_size = 0;
    FixedRangeTab* pendding_range = nullptr;
    Usage pendding_range_usage;
    for(auto range : vinfo_->prefix2range){
      Usage range_usage = range.second.RangeUsage();
      if(max_range_size < range_usage.range_size){
        pendding_range = range.second;
        pendding_range_usage = range_usage;
      }
    }

    CompactionItem* compaction_item = new CompactionItem;
    compaction_item->pending_compated_range_ = pendding_range;
    compaction_item->range_size_ = pendding_range_usage.range_size;
    compaction_item->chunk_num_ = pendding_range_usage.chunk_num;
    compaction_item->start_key_.DecodeFrom(pendding_range_usage.start);
    compaction_item->end_key_.DecodeFrom(pendding_range_usage.end);

    vinfo_->range_queue_.push(compaction_item);
  }
}

void FixedRangeChunkBasedNVMWriteCache::RebuildFromPersistentNode()
{
  // 遍历每个Node，重建vinfo中的prefix2range
  // 遍历pmem_hash_map中的每个tab，每个tab是一个p_node指针数组
  pmem_hash_map* vhash_map = pinfo_->range_map_.get();

  size_t hash_map_size = vhash_map->tabLen;

  for(size_t i = 0; i < hash_map_size; i++){
    p_node pnode = vhash_map->tab[i];
    if(pnode != nullptr){
      p_node tmp = pnode;
      do{
        // 重建FixedRangeTab
        FixedRangeTab* recovered_tab = new FixedRangeTab(pop_, tmp, vinfo_->internal_options_);
        p_range::Node* tmp_node = tmp.get();
        std::string prefix(tmp_node->prefix_.get(), tmp_node->prefixLen);
        vinfo_->prefix2range[prefix] = recovered_tab;
        tmp = tmp_node->next;
      }while(tmp != nullptr);
    }
  }
}

InternalIterator *FixedRangeChunkBasedNVMWriteCache::NewIterator(const InternalKeyComparator* icmp,
                                                                 Arena* arena)
{
  // TODO:NewIterator
  InternalIterator* internal_iter;
  MergeIteratorBuilder merge_iter_builder(icmp, arena);
  for (auto range : vinfo_->prefix2range) {
    merge_iter_builder.AddIterator(range.second.NewInternalIterator(icmp, arena));
  }

  internal_iter = merge_iter_builder.Finish();
  return internal_iter;
}

} // namespace rocksdb


