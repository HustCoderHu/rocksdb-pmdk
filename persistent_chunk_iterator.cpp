#include "persistent_chunk_iterator.h"

namespace rocksdb {

PersistentChunkIterator::PersistentChunkIterator(char *data,
                                                 size_t size, Arena *arena)
  :data_(data), arena_(arena)
{
  // keysize | key | valsize | val | ... | 1st pair offset
  // | 2nd pair offset | ... | num of pairs
  size_t nPairs;
  char* nPairsOffset = data + size - sizeof(nPairs);
  nPairs = *(reinterpret_cast<size_t*>(nPairsOffset));
  vKey_.reserve(nPairs);
  vValue_.reserve(nPairs);

  char* metaOffset = nPairsOffset - sizeof(metaOffset) * nPairs;// 0 first

  for (int i = 0; i < nPairs; ++i) {
    size_t pairOffset = *(reinterpret_cast<size_t*>(metaOffset));
    char *pairAddr = data + pairOffset;

    // key
    size_t _size = *(reinterpret_cast<size_t*>(pairAddr));
    vKey_.emplace_back(pairAddr + sizeof(_size), _size);

    pairAddr += sizeof(_size) + _size;
    // value
    _size = *(reinterpret_cast<size_t*>(pairAddr));
    vValue_.emplace_back(pairAddr + sizeof(_size), _size);

    // next pair
    metaOffset += sizeof(pairOffset);
  }
}

} // namespace rocksdb

