#ifndef PMEM_HASH_MAP_H
#define PMEM_HASH_MAP_H

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <string.h>

namespace p_range {
using namespace pmem;
using namespace pmem::obj;

struct Node {
  p<uint64_t> hash_;
  persistent_ptr<Node> next;
  p<size_t> prefixLen;
  persistent_ptr<char[]> prefix_;
  persistent_ptr<char[]> p_range;
};

using p_node = persistent_ptr<Node>;

class pmem_hash_map {
public:
//  struct p_map {
    p<uint32_t> tabLen;
    persistent_ptr<p_node[]> tab;
    p<uint32_t> threshold;
    p<uint32_t> size;
//  };
//  persistent_ptr<p_map> map_ = nullptr;


  p_node get(size_t idx) {
    return idx < size ? tab[idx] : nullptr;
  }

  using std::string;
  uint64_t put(pool_base& pop, const string& prefix);
};

} // end of namespace p_range

#endif // PMEM_HASH_MAP_H
