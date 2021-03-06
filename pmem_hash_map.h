#ifndef PMEM_HASH_MAP_H
#define PMEM_HASH_MAP_H

#include <string>
#include <vector>

#include "libpmemobj++/p.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/transaction.hpp"
#include "libpmemobj++/make_persistent.hpp"
#include "libpmemobj++/make_persistent_array.hpp"

namespace p_range {
//using pmem;
using std::string;
using std::vector;

using pmem::obj::pool_base;
using pmem::obj::pool;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;
using pmem::obj::transaction;

#ifdef DEBUG_PMEM_HASH_MAP
  #define MY_PRINT(fmt, args...)
    printf("%s [%s:%d] " fmt, __FILE__, __func__, __LINE__, ##args)
#else
  #define MY_PRINT(fmt, args...)
#endif

template <typename T>
class pmem_hash_map {
  struct Node2 {
    persistent_ptr<Node2> next;
    persistent_ptr<T> p_content;
  };
  using p_node_t = persistent_ptr<Node2>;
public:
  pmem_hash_map(pool_base &pop, float loadFactor, uint64_t tabLen)
  {
    transaction::run(pop, [&]{
      tabLen_ = tabLen;
      tab_ = make_persistent<p_node_t[]>(tabLen);
      loadFactor_ = loadFactor;
      threshold_ = tabLen_ * loadFactor_;
      size_ = 0;
    });
  }
  void init(pool_base &pop, float loadFactor, uint64_t tabLen)
  {
    transaction::run(pop, [&]{
      tabLen_ = tabLen;
      tab_ = make_persistent<p_node_t[]>(tabLen);
      loadFactor_ = loadFactor;
      threshold_ = tabLen_ * loadFactor_;
      size_ = 0;
    });
  }

  void getAll(vector<persistent_ptr<T> > &contentVec)
  {
    size_t tablen = tabLen_;
    for (size_t i = 0; i < tablen; ++i) {
      p_node_t node = tab_[i];

      while (node != nullptr) {
        contentVec.push_back(node->p_content);
        node = node->next;
      }
    }
  }

  void put(pool_base& pop, persistent_ptr<T> &p_content)
  {
    // 调用者自己构建 map ，检查是否已经有同样的 key
    MY_PRINT("\n");
    uint64_t _hash = p_content->hashCode();
    MY_PRINT("\n");
    uint64_t tabLen = tabLen_.get_ro();
    MY_PRINT("\n");
    _hash = _hash % tabLen;
    MY_PRINT("\n");
    ptrdiff_t idx = static_cast<ptrdiff_t>(_hash);
    MY_PRINT("\n");
//    p_node_t bucketHeadNode = tab_[idx];
    p_node_t bucketHeadNode = tab_[_hash % tabLen_.get_ro()];
    MY_PRINT("\n");

    p_node_t newhead;
    transaction::run(pop, [&] {
      MY_PRINT("\n");
      newhead = make_persistent<Node2>();
      MY_PRINT("\n");
      newhead->p_content = p_content;
      MY_PRINT("\n");
      newhead->next = bucketHeadNode;
      MY_PRINT("\n");
      tab_[_hash % tabLen_] = newhead;
      MY_PRINT("\n");
    });
  }
  //  persistent_ptr<char[]> get(const std::string& key, size_t prefixLen);
  //  p_node getNode(const std::string& key, size_t prefixLen);
  //  p_node getNode(uint64_t hash, const std::string& key);
  //  using std::string;
  //  uint64_t put(pool_base& pop, const string& prefix, size_t bufSize);
  //  p_node putAndGet(pool_base& pop, const string& prefix, size_t bufSize);
//public:
  p<uint32_t> tabLen_;
//  persistent_ptr<p_node[]> tab;
  persistent_ptr<p_node_t[]> tab_;

  p<float> loadFactor_;
  p<uint32_t> threshold_;
  p<uint32_t> size_;
};

} // end of namespace p_range

#endif // PMEM_HASH_MAP_H
