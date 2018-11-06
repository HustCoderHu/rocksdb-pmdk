#ifndef FIXED_RANGE_ITERATOR_H
#define FIXED_RANGE_ITERATOR_H

#include <cstdint>

#include "table/internal_iterator.h"

namespace rocksdb {

// 参考 testutil.h

class FixRangeIterator:public InternalIterator
{
    public:
    explicit FixRangeIterator(size_t bloomFilterLen) {

    }

    virtual bool Valid() const override { return current_ < keys_.size(); }


};

} // namespace rocksdb

#endif // FIXED_RANGE_ITERATOR_H
