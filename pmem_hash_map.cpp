#include "pmem_hash_map.h"

//#include "city.h"
// https://blog.csdn.net/yfkiss/article/details/7337382

namespace p_range {

pmem_hash_map::pmem_hash_map()
{

}

uint64_t pmem_hash_map::put(pool_base &pop, const std::string &prefix) {
  // ( const void * key, int len, unsigned int seed );
  uint64_t _hash = CityHash64WithSeed(prefix, prefix.size(), 16);

  p_node nod = tab[_hash % tabLen];

  p_node tmp = nod;
  while (tmp != nullptr) {
    if (tmp->hash_ == _hash
        && strcmp(tmp->prefix_, prefix.c_str()) == 0)
      break;
    tmp = tmp->next;
  }

  // 前缀没有被插入过
  if (nullptr == tmp) {
    transaction::run(pop, [&] {
      p_node newhead = make_persistent<p_node>();

      newhead->hash_ = _hash;
      newhead->prefixLen = prefix.size();
      newhead->prefix_ = make_persistent<char[]>(prefix.size()+1);
      memcpy(newhead->prefix_, prefix.c_str(), prefix.size()+1);

      // TODO
      // p_range = ?  range 分配多大空间

      newhead->next = nod;
      tab[_hash % tabLen] = newhead;
    });
  }
  else {
    // 已经插入过了
  }
  return _hash;
}

} // end of namespace p_range
