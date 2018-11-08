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
    char* end = data + size - sizeof(nPairs);
    nPairs = *(const_cast<size_t*>(end));
    pair_offset.reserve(nPairs);

    char* start = end - sizeof(size_t) * nPairs;

    for (int i = 0; i < nPairs; ++i) {
      pair_offset.push_back(start);
      start += sizeof(size_t);
    }
  }

  Slice Key() override {
    char *pairOffset = data_ + pair_offset.at(current_);
    size_t keySize = *(const_cast<size_t*>(pairOffset));

    char *dest;
    memcpy(dest, pairOffset + sizeof(keySize), keySize);
  }
  Slice Value() override {
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
  vector<char*> pair_offset;
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
