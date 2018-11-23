#ifndef PMEM_HASH_MAP_H
#define PMEM_HASH_MAP_H

#include <string.h>
#include <vector>


#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>


namespace p_range {
//using pmem;
using std::string;
using std::vector;

using pmem::obj::pool_base;
using pmem::obj::pool;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;


// deprecated
struct Node {
  p<uint64_t> hash_;
  persistent_ptr<Node> next;
  p<size_t> prefixLen; // string prefix_ tail 0 not included
  persistent_ptr<char[]> prefix_;
  persistent_ptr<char[]> key_range_;
  p<size_t> chunk_num_;
  p<uint64_t> seq_num_;

  p<size_t> bufSize; // capacity
  persistent_ptr<char[]> buf;
  p<size_t> dataLen; // exact data len
};

//template <typename T>
//struct Node2 {
//  using p_node_t = persistent_ptr<Node2<T>>;
//  p_node_t next;
////  persistent_ptr<Node2<T>> next;
//  persistent_ptr<T> p_content;
//};

//typedef persistent_ptr<Node> p_node;
using p_node = persistent_ptr<Node>;

template <typename T>
class pmem_hash_map {
  using p_node_t = persistent_ptr<Node2<T>>;

  struct Node2 {
    p_node_t next;
  //  persistent_ptr<Node2<T>> next;
    persistent_ptr<T> p_content;
  };

  void getAll(vector<persistent_ptr<T> > &nodeVec);
  bool put(pool_base& pop, persistent_ptr<T> &p_content);

  persistent_ptr<char[]> get(const std::string& key, size_t prefixLen);

  p_node getNode(const std::string& key, size_t prefixLen);
  p_node getNode(uint64_t hash, const std::string& key);

  using std::string;
  uint64_t put(pool_base& pop, const string& prefix, size_t bufSize);
  p_node putAndGet(pool_base& pop, const string& prefix, size_t bufSize);

public:
  //  struct p_map {
  p<uint32_t> tabLen;
  persistent_ptr<p_node[]> tab;
  persistent_ptr<p_node_t[]> tab_;

  p<float> loadFactor;
  p<uint32_t> threshold;
  p<uint32_t> size;
  //  };
  //  persistent_ptr<p_map> map_ = nullptr;


};

} // end of namespace p_range

#endif // PMEM_HASH_MAP_H
