#ifndef PERSISTENT_RANGE_MEM_SET_H
#define PERSISTENT_RANGE_MEM_SET_H

#include <rocksdb/iterator.h>

#include <persistent_range_mem.h>

namespace rocksdb {

class PersistentRangeMemSet
{
public:
    PersistentRangeMemSet();

public:
    // 返回所有RangeMemtable的有序序列
    // 基于MergeIterator
    Iterator* NewIterator();

    // 获取range_mem_id对应的RangeMemtable结构
    PersistentRangeMem* GetRangeMemtable(uint64_t range_mem_id);

    // 根据key查找value
    Status Get(const Slice& key, std::string *value);
private:
    persistent_queue<PersistentRangeMem> range_mem_list_;
    persistent_queue<uint64_t> compact_queue_;


private:
};

} // namespace rocksdb
#endif // PERSISTENT_RANGE_MEM_SET_H
