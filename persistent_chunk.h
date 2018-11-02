#ifndef PERSISTENT_CHUNK_H
#define PERSISTENT_CHUNK_H

#include <rocksdb/iterator.h>
#include <table/merging_iterator.h>

namespace rocksdb {
class ChunkIterator {

};

class PersistentChunk
{
public:
    PersistentChunk();

    InternalIterator* NewInternalIterator();

    static PersistentChunk* parseFromRaw(const Slice& slc);
};
} // namespace rocksdb
#endif // PERSISTENT_CHUNK_H
