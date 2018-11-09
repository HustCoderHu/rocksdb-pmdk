#ifndef PERSISTENT_CHUNK_H
#define PERSISTENT_CHUNK_H

#include <vector>

#include <rocksdb/iterator.h>
#include <table/merging_iterator.h>

namespace rocksdb {
using std::vector;

class ChunkIterator : public InternalIterator {
  explicit ChunkIterator(char *data, size_t size)
    :data_(data)
  {
    // keysize | key | valsize | val | ... | 1st pair offset
    // | 2nd pair offset | ... | num of pairs
    size_t nPairs;
    char* nPairsOffset = data + size - sizeof(nPairs);
    nPairs = *(static_cast<size_t*>(nPairsOffset));
    vKey_.reserve(nPairs);
    vValue_.reserve(nPairs);

    char* metaOffset = nPairsOffset - sizeof(metaOffset) * nPairs;

    for (int i = 0; i < nPairs; ++i) {
      size_t pairOffset = *(static_cast<size_t*>(metaOffset));
      char *pairAddr = data + pairOffset;

      // key
      size_t _size = *(static_cast<size_t*>(pairAddr));
      vKey_.emplace_back(pairAddr + sizeof(_size), _size);

      pairAddr += sizeof(_size) + _size;
      // value
      _size = *(static_cast<size_t*>(pairAddr));
      vValue_.emplace_back(pairAddr + sizeof(_size), _size);

      // next pair
      metaOffset += sizeof(pairOffset);
    }
  }

  Slice Key() override {
    // TODO
    // 读取一致性问题 是否要事务 防止flush时只读取到部分数据
    char *pairOffset = data_ + pair_offset.at(current_);
    size_t keySize = *(const_cast<size_t*>(pairOffset));

    char *dest;
    memcpy(dest, pairOffset + sizeof(keySize), keySize);
  }

  Slice Value() override {
    // TODO
    // 同 Key()
    char *pairOffset = data_ + pair_offset.at(current_);
    size_t keySize = *(const_cast<size_t*>(pairOffset));

    char *valsizeOffset = pairOffset + sizeof(keySize) + keySize;
    size_t valSize = *(const_cast<size_t*>(valsizeOffset));

    char *dest;
    memcpy(dest, valsizeOffset + sizeof(valSize), valSize);
  }
  bool Valid() override { return 0 <= current_ && current_ < pair_offset.size(); }
  void Next() override { ++current_; }
  void Prev() override { --current_; }

  ~ChunkIterator();

  char *data_; // 数据起点
//  vector<char*> pair_offset;
  vector<Slice> vKey_;
  vector<Slice> vValue_;
  size_t current_;
//  size_t nPairs;
};

class PersistentChunk
{
public:
  PersistentChunk(size_t bloomFilterSize, size_t chunkSize,
                  size_t chunkData)
    :bloomFilterSize_(bloomFilterSize), chunkSize_(chunkSize),
      chunkData_(chunkData) {

  }

  InternalIterator *NewInternalIterator() {
    ChunkIterator *iter = new ChunkIterator(chunkData_, chunkSize_);
    return iter;
  }

  static PersistentChunk* parseFromRaw(const Slice& slc);

  size_t bloomFilterSize_;
  size_t chunkSize_;
  size_t chunkData_;
};
} // namespace rocksdb
#endif // PERSISTENT_CHUNK_H
