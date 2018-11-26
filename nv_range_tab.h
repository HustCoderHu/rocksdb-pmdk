#ifndef NV_RANGE_TAB_H
#define NV_RANGE_TAB_H

#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/transaction.hpp"
#include "libpmemobj++/make_persistent.hpp"

namespace rocksdb {

class NvRangeTab
{
  using std::string;
  using pmem::obj::p;
  using pmem::obj::pool_base;
  using pmem::obj::pool;
  using pmem::obj::persistent_ptr;
  using pmem::obj::make_persistent;
  using pmem::obj::transaction;

  using p_buf = persistent_ptr<char[]>;

public:
   NvRangeTab(pool_base &pop, const string &prefix, uint64_t range_size)
  {
    transaction::run(pop, [&] {
      prefix_ = make_persistent<char[]>(prefix.size());
      memcpy(prefix_.get(), prefix.c_str(), prefix.size());

      key_range_ = nullptr;
      extra_buf = nullptr;
      buf = make_persistent<char[]>(range_size);

      hash_ = hashCode(prefix);
      prefixLen = prefix.size();
      chunk_num_ = 0;
      seq_num_ = 0;
      bufSize = range_size;
      dataLen = 0;
    });
  }

  uint64_t hashCode(const string& prefix) {
    // TODO \0 ?
//    string key(prefix_.get(), prefixLen + 1);
//    return CityHash64WithSeed(key, prefixLen, 16);
    return CityHash64WithSeed(prefix, prefix.size(), 16);
  }

  char* GetRawBuf(){return buf.get();}

  // 通过比价前缀，比较两个NvRangeTab是否相等
  bool equals(const string &prefix);

  bool equals(p_buf &prefix, size_t len);

  bool equals(NvRangeTab &b);

  p<uint64_t> hash_;
  p<size_t> prefixLen; // string prefix_ tail 0 not included
  p_buf prefix_;
  p_buf key_range_;
  p<size_t> chunk_num_;
  p<uint64_t> seq_num_;

  p<size_t> bufSize; // capacity
  p_buf buf;
  p<size_t> dataLen; // exact data len

  persistent_ptr<NvRangeTab> extra_buf;
};

} // namespace rocksdb

#endif // NV_RANGE_TAB_H
