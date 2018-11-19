#ifndef CHUNKBLK_H
#define CHUNKBLK_H

#include <stdint.h>

namespace rocksdb {

class ChunkBlk
{
public:
//  | chunk blmFilter | chunk len | data ...   |  不定长
  explicit ChunkBlk(size_t offset, size_t chunkLen)
    :offset_(offset), chunkLen_(chunkLen) {

  }

  size_t getDatOffset() {
    return offset_ + CHUNK_BLOOM_FILTER_SIZE + sizeof(chunkLen_);
  }

  size_t offset_; // offset of bloom filter in range buffer
  size_t chunkLen_;

  // kv data start at offset + CHUNK_BLOOM_FILTER_SIZE + sizeof(chunLen)
};

} // namespace rocksdb

#endif // CHUNKBLK_H
