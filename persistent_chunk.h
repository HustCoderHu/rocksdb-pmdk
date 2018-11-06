#ifndef PERSISTENT_CHUNK_H
#define PERSISTENT_CHUNK_H

#include <vector>

#include <rocksdb/iterator.h>
#include <table/merging_iterator.h>

namespace rocksdb {
using std::vector;

class ChunkIterator {
    explicit ChunkIterator(const Slice& slc)
    {

        char* end = slc.data_ + slc.size_ - sizeof(nPairs);
        nPairs = *(const_cast<size_t*>(end));
        pair_offset.reserve(nPairs);

        char* start = end - sizeof(size_t) * nPairs;

        for (int i = 0; i < nPairs; ++i) {
           pair_offset.push_back(start);
           start += sizeof(size_t);
        }
    }

    ~ChunkIterator();

    vector<char*> pair_offset;
    size_t current_;
    size_t nPairs;
};

class PersistentChunk
{
public:

    // keysize | key | valsize | val | ... | 1st pair offset
    // | 2nd pair offset | ... | num of pairs
    PersistentChunk();

    InternalIterator* NewInternalIterator();

    static PersistentChunk* parseFromRaw(const Slice& slc);
};
} // namespace rocksdb
#endif // PERSISTENT_CHUNK_H
