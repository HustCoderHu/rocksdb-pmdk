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

FixedRangeChunkBasedNVMWriteCache::FixedRangeChunkBasedNVMWriteCache(const string& file, uint64_t pmem_size)
  :file_path(file)
  ,LAYOUT("FixedRangeChunkBasedNVMWriteCache")
  ,POOLSIZE(1 << 26) // TODO
{
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
      pinfo_->allocator_ = make_persistent<PersistentAllocator>();

      pinfo_->inited_ = true;
    });
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

void FixedRangeChunkBasedNVMWriteCache::addCompactionRangeTab(FixedRangeTab *tab)
{
  //  range_queue_.pu
}

uint64_t FixedRangeChunkBasedNVMWriteCache::NewRange(const std::string &prefix)
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
}

void FixedRangeChunkBasedNVMWriteCache::AppendToRange(FixedRangeTab *tab,
                                                      const char *bloom_data,
                                                      const Slice &chunk_data,
                                                      const Slice &new_start,
                                                      const Slice &new_end)
{
  tab->Append(pop_, bloom_data, chunk_data, new_start, new_end);
}
} // namespace rocksdb


