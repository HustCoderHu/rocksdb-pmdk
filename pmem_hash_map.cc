#include "pmem_hash_map.h"

using pmem::obj::transaction;

//#include "city.h"
// https://blog.csdn.net/yfkiss/article/details/7337382

namespace p_range {

pmem_hash_map::pmem_hash_map()
{

}

template<typename T>
void pmem_hash_map::getAll(vector<persistent_ptr<T> > &nodeVec)
{
  size_t tablen = tabLen;
  for (int i = 0; i < tablen; ++i) {
    p_node_t node = tab_[i];

    while (node != nullptr) {
      nodeVec.push_back(node->p_content);
      node = node->next;
    }
  }
}

template<typename T>
bool pmem_hash_map::put(pmem::obj::pool_base &pop, persistent_ptr<T> &p_content)
{
  // 调用者自己构建 map ，检查是否已经有同样的 key
  uint64_t _hash = p_content->hashCode();
  p_node_t bucketHeadNode = tab_[_hash % tabLen];

  p_node_t newhead;
  if (nullptr == bucketHeadNode) {
    transaction::run(pop, [&] {
      newhead = make_persistent<p_node>();
      newhead->p_content = p_content;
      newhead->next = nullptr;
      tab[_hash % tabLen] = newhead;
    });
  }
  return true;
}

persistent_ptr<char[]> pmem_hash_map::get(const std::string &key, size_t prefixLen)
{
  p_node nod;

//  uint64_t _hash = CityHash64WithSeed(key, prefixLen, 16);
//  nod = getNode(_hash, key);
  node = getNode(key, prefixLen);

  return nod == nullptr ? nullptr : nod->buf;
  // TODO
  // bufSize 需要么
}

p_node pmem_hash_map::getNode(const std::string &key, size_t prefixLen)
{
  uint64_t _hash = CityHash64WithSeed(key, prefixLen, 16);
  return getNode(_hash, key);
}

p_node pmem_hash_map::getNode(uint64_t hash, const std::string &key)
{
  p_node nod = tab[hash % tabLen];

  p_node tmp = nod;
  while (tmp != nullptr) {
    if (tmp->hash_ == hash
        && strcmp(tmp->prefix_, key.c_str()) == 0)
      break;
    tmp = tmp->next;
  }
  return tmp;
}

uint64_t pmem_hash_map::put(pool_base &pop, const std::string &prefix,
                            size_t bufSize) {
  return putAndGet(pop, prefix, bufSize)->hash_;
}

p_node pmem_hash_map::putAndGet(pool_base &pop, const std::string &prefix, size_t bufSize)
{
  // ( const void * key, int len, unsigned int seed );
  uint64_t _hash = CityHash64WithSeed(prefix, prefix.length(), 16);

  p_node bucketHeadNode = tab[_hash % tabLen];

  p_node try2find = bucketHeadNode;
  while (try2find != nullptr) {
    if (try2find->hash_ == _hash
        && strcmp(try2find->prefix_, prefix.c_str()) == 0)
      break;
    try2find = try2find->next;
  }

  p_node newhead = try2find;
  // 前缀没有被插入过
  if (nullptr == try2find) {
    transaction::run(pop, [&] {
      newhead = make_persistent<p_node>();

      newhead->hash_ = _hash;
      newhead->prefixLen = prefix.length();
      newhead->prefix_ = make_persistent<char[]>(prefix.length()+1);
      memcpy(newhead->prefix_, prefix.c_str(), prefix.length()+1);

      // TODO
      // key_range_ ?
      // seq_num_ ?
      newhead->key_range_ = nullptr;
      newhead->chunk_num_ = 0;
      newhead->seq_num_ = ?;

      newhead->bufSize = bufSize
      newhead->buf = make_persistent<char[]>(bufSize);
      newhead->dataLen = 0;

      newhead->next = bucketHeadNode;
      tab[_hash % tabLen] = newhead;
    });
  } else {
    // 已经插入过了
  }
  return newhead;
}

} // end of namespace p_range
