#include "fixed_range_tab.h"
#include <persistent_chunk.h>

#include <table/merging_iterator.h>
#include <libpmemobj++/transaction.hpp>

namespace rocksdb {

using pmem::obj::pool_base;
using pmem::obj::pool;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;
using pmem::obj::transaction;

using p_range::p_node;
#define MAX_BUF_LEN 4096

//POBJ_LAYOUT_BEGIN(range_mem);
//POBJ_LAYOUT_ROOT(range_mem, struct my_root);
//POBJ_LAYOUT_TOID(range_mem, struct foo_el);
//POBJ_LAYOUT_TOID(range_mem, struct bar_el);
//POBJ_LAYOUT_END(range_mem);

struct my_root {
  size_t length; // mark end of chunk block sequence
  unsigned char data[MAX_BUF_LEN];
};



FixedRangeTab::FixedRangeTab(pool_base *pop, p_range::p_node node_in_pmem_map,
                             FixedRangeBasedOptions *options)
  : node_in_pmem_map_(node_in_pmem_map)
  , interal_options_(options)
  //  , chunk_sum_size(0)
  //  , MAX_CHUNK_SUM_SIZE(64 * 1024 * 1024)
  //  , pop_(pop)
  //  , node_in_pmem_map_(node_in_pmem_map)
{
  buildBlkList();

  transaction::run(pop, [&]{
    // TODO：range的初始化
    range_info_ = make_persistent<freqUpdateInfo>(node_in_pmem_map->bufSize);
    range_info_->real_start_ = nullptr;
    range_info_->real_end_ = nullptr;
    range_info_->chunk_num = 0;
    range_info_->seq_num_ = 0;
    range_info_->total_size_ = 0;
  });

  // set cur_
  // set seq_
  in_compaction_ = false;
}

FixedRangeTab::~FixedRangeTab()
{

}

//| prefix data | prefix size |
//| cur_ | seq_ |
//| chunk blmFilter | chunk len | chunk data ..| 不定长
//| chunk blmFilter | chunk len | chunk data ...| 不定长
//| chunk blmFilter | chunk len | chunk data .| 不定长


InternalIterator* FixedRangeTab::NewInternalIterator(
    ColumnFamilyData *cfd, Arena *arena)
{
  InternalIterator* internal_iter;
  MergeIteratorBuilder merge_iter_builder(&cfd->internal_comparator(),
                                          arena);
  // TODO
  // 预设 range 持久化
  //  char *chunkBlkOffset = data_ + sizeof(stat.used_bits_) + sizeof(stat.start_)
  //      + sizeof(stat.end_);
  PersistentChunk pchk;
  for (ChunkBlk &blk : blklist) {
    pchk.reset(CHUNK_BLOOM_FILTER_LEN, blk.chunkLen_,
               node_in_pmem_map_->buf + blk.getDatOffset());
    merge_iter_builder.AddIterator(pchk.NewIterator(arena));
  }

  internal_iter = merge_iter_builder.Finish();
}

Status FixedRangeTab::Get(const InternalKeyComparator &internal_comparator,
                          const Slice &key, std::string *value)
{
  // TODO
  for (ChunkBlk &blk : blklist) {
    persistent_ptr<char[]> datFilter = node_in_pmem_map_->buf + blk.offset_;

    // check bloom

    size_t chunkLen = blk.chunkLen_;
    persistent_ptr<char[]> exactChunk = node_in_pmem_map_->buf + blk.getDatOffset();
  }

  // return ?;

  // 1.从下往上遍历所有的chunk
  uint64_t bloom_bits = interal_options_->chunk_bloom_bits_;
  for (size_t i = chunk_offset_.size() - 1; i >= 0; i--) {
    uint64_t off = chunk_offset_[i];
    char *pchunk_data = raw_ + off;
    char *chunk_start = pchunk_data;
    // 2.获取当前chunk的bloom data，查找这个bloom data判断是否包含对应的key
    if (interal_options_->filter_policy_->KeyMayMatch(key, Slice(chunk_start, bloom_bits))) {
      // 3.如果有则读取元数据进行chunk内的查找
      uint64_t chunk_len = DecodeFixed64(chunk_start + bloom_bits);
      chunk_start += (bloom_bits + sizeof(uint64_t) + chunk_len);
      uint64_t item_num = DecodeFixed64(chunk_start - sizeof(uint64_t));
      chunk_start -= (item_num + 1) * sizeof(uint64_t);
      std::vector<uint64_t> item_offs;
      while (item_num-- > 0) {
        item_offs.push_back(DecodeFixed64(chunk_start));
        chunk_start += sizeof(uint64_t);
      }
      Status s = DoInChunkSearch(key, value, item_offs, pchunk_data + bloom_bits + sizeof(uint64_t));
      if (s.ok()) return s;
    } else {
      continue;
    }
  }

  // 4.循环直到查找完所有的chunk
}

void FixedRangeTab::buildBlkList()
{
  size_t dataLen;
  dataLen = node_in_pmem_map_->dataLen;

  // TODO
  // range 从一开始就存 chunk ?
  size_t offset = 0;
  while(offset < dataLen) {
    size_t chunkLenOffset = offset + CHUNK_BLOOM_FILTER_LEN;
    size_t chunkLen;
    memcpy(&chunkLen, node_in_pmem_map_->buf + chunkLenOffset, sizeof(chunkLen));
    blklist.emplace_back(offset, chunkLen);
    // next chunk block
    offset += CHUNK_BLOOM_FILTER_LEN + sizeof(chunkLen) + chunkLen;
  }
}

/* range data format:
 *
 * |--  cur_  --|
 * |--  seq_  --|
 * |-- chunk1 --|
 * |-- chunk2 --|
 * |--   ...  --|
 *
 * */

void FixedRangeTab::Append(const char *bloom_data,
                           const Slice &chunk_data,
                           const Slice &new_start,
                           const Slice &new_end)
{
  if (chunk_sum_size + chunk_data.size_ >= MAX_CHUNK_SUM_SIZE
      || info.chunk_num > ) {
    // TODO
    // 触发 compaction
    chunk_sum_size = 0;
  }

  // 开始追加
  transaction::run(pop_, [&] {
    size_t cur_len = node_in_pmem_map_->dataLen;
    blklist.emplace_back(cur_len, chunk_data.size_);

    persistent_ptr<char[]> dest = node_in_pmem_map_->buf + cur_len;
    memcpy(dest, bloom_data, CHUNK_BLOOM_FILTER_LEN);
    memcpy(dest+CHUNK_BLOOM_FILTER_LEN, &chunk_data.size_, sizeof(chunk_data.size_));
    dest += CHUNK_BLOOM_FILTER_LEN+sizeof(chunk_data.size_);
    memcpy(dest, chunk_data.data_, chunk_data.size_);
    node_in_pmem_map_->dataLen = cur_len + CHUNK_BLOOM_FILTER_LEN + sizeof(chunk_data.size_)
        + chunk_data.size_;
  });

  return ;

  // TODO
  // 事务更换成 cpp binding ?
  //  TX_BEGIN(pop) {
  /* TX_STAGE_WORK */
  //    rootp = POBJ_ROOT(pop, my_root);
  size_t cur_len = node_in_pmem_map_->dataLen;
  size_t chunk_blk_len = CHUNK_BLOOM_FILTER_LEN + sizeof(chunk_data.size_)
      + chunk_data.size_;
  // 添加持久化范围
  //    TX_ADD_FIELD(rootp, length);
  unsigned char *dest = node_in_pmem_map_->buf + cur_len;
  pmemobj_tx_add_range_direct(dest, chunk_blk_len);
  // 复制 chunk block
  memcpy(dest, bloom_data, CHUNK_BLOOM_FILTER_LEN);
  memcpy(dest+CHUNK_BLOOM_FILTER_LEN, &chunk_data.size_, sizeof(chunk_data.size_));
  dest += CHUNK_BLOOM_FILTER_LEN+sizeof(chunk_data.size_);
  memcpy(dest, chunk_data.data_, chunk_data.size_);
  // 更新总长度
  node_in_pmem_map_->dataLen = cur_len + chunk_blk_len;
  chunk_sum_size += chunk_data.size_;
  // blk 偏移
  //    psttChunkList.push_back(cur_len);

  info.chunk_num++;
  //        dest += chunk_data.size_;
  //        info.update(new_start, new_end);
  //        memcpy(dest, info, sizeof)
  //    } TX_ONCOMMIT {
  //        /* TX_STAGE_ONCOMMIT */
  //    } TX_ONABORT {
  //        /* TX_STAGE_ONABORT */
  //    } TX_FINALLY {
  //        /* TX_STAGE_FINALLY */
  //  } TX_END
}

void FixedRangeTab::GetRealRange(Slice &real_start, Slice &real_end)
{
  real_start = GetKVData(&range_info_->real_start_[0], 0);
  real_end = GetKVData(&range_info_->real_end_[0], 0);
}

Status FixedRangeTab::DoInChunkSearch(InternalKeyComparator &icmp, const Slice &key, std::string *value,
                                      std::vector<uint64_t> &off, char *chunk_data)
{
  size_t left = 0, right = off.size() - 1;
  size_t middle = (left + right) / 2;
  while (left < right) {
    Slice ml_key = GetKVData(chunk_data, off[middle]);
    int result = icmp.Compare(ml_key, key);
    if (result == 0) {
      //found
      uint64_t value_off = DecodeFixed64(chunk_data + off[middle] + ml_key.size());
      Slice raw_value = GetKVData(chunk_data, value_off);
      value = new std::string(raw_value.data(), raw_value.size());
      return Status::OK();
    } else if (result < 0) {
      // middle < key
      left = middle + 1;
    } else if (result > 0) {
      // middle >= key
      right = middle - 1;
    }
  }
  return Status::NotFound("not found");
}

Slice FixedRangeTab::GetKVData(char *raw, uint64_t item_off)
{
  char* target = raw + item_off;
  uint64_t target_size = DecodeFixed64(target);
  return Slice(target + sizeof(uint64_t), target_size);
}

void FixedRangeTab::Release()
{

}

} // namespace rocksdb
