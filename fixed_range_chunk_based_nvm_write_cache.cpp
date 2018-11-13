#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "fixed_range_chunk_based_nvm_write_cache.h"

#include <ex_common.h>

namespace rocksdb {

using p_range::pmem_hashTab;

FixedRangeChunkBasedNVMWriteCache::FixedRangeChunkBasedNVMWriteCache(const string& file, const string& layout)
  :file_path(file)
  ,LAYOUT(layout)
  ,POOLSIZE(1 << 26)
{
  //    file_path.data()
  bool justCreate = false;
  if (file_exists(file_path) != 0) {
    pop = pool<pmem_hashTab>::create(file_path, LAYOUT, POOLSIZE, CREATE_MODE_RW);
    justCreate = true;

  } else {
    pop = pool<pmem_hashTab>::open(file_path, LAYOUT);
  }
//  pmem_ptr = pop.root();

  transaction::run(pop, [&] {
    persistent_ptr<p_range::pmem_hashTab> pmem_ptr = pop.root();
    if ()
  });

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
  if (pop)
    pop.close();
//    pmemobj_close(pop);
}

Status FixedRangeChunkBasedNVMWriteCache::Get(const Slice &key, std::string *value)
{
  FixedRangeTab *tab;

  // TODO
  // 1. calc target FixedRangeTab
  tab;
  // 2.
  return tab->Get(key, value);
}

uint64_t FixedRangeChunkBasedNVMWriteCache::NewRange(const std::string &prefix)
{
  // ( const void * key, int len, unsigned int seed );
//  uint64_ _hash = CityHash64WithSeed(prefix, prefix.size(), 16);
}

} // namespace rocksdb


