#include "fixed_range_tab.h"
#include <persistent_chunk.h>

#include <memory>

#include <table/merging_iterator.h>
#include <libpmemobj++/transaction.hpp>

namespace rocksdb {

using std::shared_ptr;

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

//struct my_root {
//  size_t length; // mark end of chunk block sequence
//  unsigned char data[MAX_BUF_LEN];
//};

FixedRangeTab::FixedRangeTab(pool_base& pop, p_node node_in_pmem_map,
                             FixedRangeBasedOptions *options)
  : pmap_node_(node_in_pmem_map)
  , pop_(pop)
  , interal_options_(options)
  //  , chunk_sum_size(0)
  //  , MAX_CHUNK_SUM_SIZE(64 * 1024 * 1024)
  //  , pop_(pop)
  //  , node_in_pmem_map_(node_in_pmem_map)
{
  RebuildBlkList();

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
    const InternalKeyComparator* icmp, Arena *arena)
{
  InternalIterator* internal_iter;
  MergeIteratorBuilder merge_iter_builder(icmp, arena);
  // TODO
  // 预设 range 持久化
  //  char *chunkBlkOffset = data_ + sizeof(stat.used_bits_) + sizeof(stat.start_)
  //      + sizeof(stat.end_);
  PersistentChunk pchk;
  for (ChunkBlk &blk : blklist) {
    pchk.reset(interal_options_->chunk_bloom_bits_, blk.chunkLen_,
               pmap_node_->buf + blk.getDatOffset());
    merge_iter_builder.AddIterator(pchk.NewIterator(arena));
  }

  internal_iter = merge_iter_builder.Finish();
  return internal_iter;
}


void FixedRangeTab::Append(const InternalKeyComparator &icmp,
                           const char *bloom_data, const Slice &chunk_data,
                           const Slice &new_start, const Slice &new_end)
{
  if (chunk_sum_size + chunk_data.size_ >= MAX_CHUNK_SUM_SIZE
      || info.chunk_num > ) {
    // TODO
    // 触发 compaction
    chunk_sum_size = 0;
  }

  // 开始追加
  transaction::run(pop_, [&] {
    size_t cur_len = pmap_node_->dataLen;
    blklist.emplace_back(cur_len, chunk_data.size_);

    persistent_ptr<char[]> dest = pmap_node_->buf + cur_len;
    memcpy(dest.get(), bloom_data, CHUNK_BLOOM_FILTER_LEN);
    dest += CHUNK_BLOOM_FILTER_LEN;
    EncodeFixed64(dest.get(), chunk_data.size_);

    dest += sizeof(chunk_data.size_);
    memcpy(dest.get(), chunk_data.data_, chunk_data.size_);
    pmap_node_->dataLen = cur_len + CHUNK_BLOOM_FILTER_LEN + sizeof(chunk_data.size_)
        + chunk_data.size_;
  });

  CheckAndUpdateKeyRange(icmp, new_start, new_end);
  return ;

  // TODO
  // 事务更换成 cpp binding ?
  //  TX_BEGIN(pop) {
  /* TX_STAGE_WORK */
  //    rootp = POBJ_ROOT(pop, my_root);
  size_t cur_len = pmap_node_->dataLen;
  size_t chunk_blk_len = CHUNK_BLOOM_FILTER_LEN + sizeof(chunk_data.size_)
      + chunk_data.size_;
  // 添加持久化范围
  //    TX_ADD_FIELD(rootp, length);
  unsigned char *dest = pmap_node_->buf + cur_len;
  pmemobj_tx_add_range_direct(dest, chunk_blk_len);
  // 复制 chunk block
  memcpy(dest, bloom_data, CHUNK_BLOOM_FILTER_LEN);
  memcpy(dest+CHUNK_BLOOM_FILTER_LEN, &chunk_data.size_, sizeof(chunk_data.size_));
  dest += CHUNK_BLOOM_FILTER_LEN+sizeof(chunk_data.size_);
  memcpy(dest, chunk_data.data_, chunk_data.size_);
  // 更新总长度
  pmap_node_->dataLen = cur_len + chunk_blk_len;
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

void FixedRangeTab::CheckAndUpdateKeyRange(const InternalKeyComparator &icmp, const Slice &new_start,
                                           const Slice &new_end)
{
  Slice cur_start, cur_end;
  bool update_start = false, update_end = false;
  GetRealRange(cur_start, cur_end);
  if(icmp.Compare(cur_start, new_start) > 0){
    cur_start = new_start;
    update_start = true;
  }

  if(icmp.Compare(cur_end, new_end) < 0){
    cur_end = new_end;
    update_end = true;
  }

  if(update_start || update_end){
    transaction::run(pop_, [&]{
      persistent_ptr<char[]> new_range = make_persistent<char[]>(cur_start.size() + cur_end.size()
                                                                 + 2 * sizeof(uint64_t));
      // get raw ptr
      char* p_new_range = new_range.get();
      // put start
      EncodeFixed64(p_new_range, cur_start.size());
      memcpy(p_new_range + sizeof(uint64_t), cur_start.data(), cur_start.size());
      // put end
      p_new_range += sizeof(uint64_t) + cur_start.size();
      EncodeFixed64(p_new_range, cur_end.size());
      memcpy(p_new_range + sizeof(uint64_t), cur_end.data(), cur_end.size());

    });
  }
}

Status FixedRangeTab::Get(const InternalKeyComparator &internal_comparator,
                          const Slice &key, std::string *value)
{
  // 1.从下往上遍历所有的chunk
  PersistentChunkIterator *iter = new PersistentChunkIterator;
  shared_ptr<PersistentChunkIterator> sp_persistent_chunk_iter(iter);

  uint64_t bloom_bits = interal_options_->chunk_bloom_bits_;
  for (size_t i = chunk_offset_.size() - 1; i >= 0; i--) {
    ChunkBlk &blk = blklist.at(i);
    persistent_ptr<char[]> bloom_dat = pmap_node_->buf + blk.offset_;
    // 2.获取当前chunk的bloom data，查找这个bloom data判断是否包含对应的key
    if (interal_options_->filter_policy_->KeyMayMatch(key, Slice(bloom_dat.get(), bloom_bits))) {
      // 3.如果有则读取元数据进行chunk内的查找
      new (iter) PersistentChunkIterator(pmap_node_->buf + blk.getDatOffset(), blk.chunkLen_,
                                         nullptr);
      Status &s = searchInChunk(iter, internal_comparator, key, value);
      if (s.ok()) return s;
    } else {
      continue;
    }
  } // 4.循环直到查找完所有的chunk
}

Status FixedRangeTab::searchInChunk(PersistentChunkIterator *iter,
                                    InternalKeyComparator &icmp,
                                    const Slice &key, std::string *value)
{
  size_t left = 0, right = iter->count() - 1;
  while (left < right) {
    size_t middle = left + ((right - left) >> 1);
    iter->SeekTo(middle);
    Slice& ml_key = iter->key();
    int result = icmp.Compare(ml_key, key);
    if (result == 0) {
      //found
      Slice& raw_value = iter->value();
      value->assign(raw_value.data(), raw_value.size());
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

void FixedRangeTab::RebuildBlkList()
{
  // TODO :check consistency
  //ConsistencyCheck();
  size_t dataLen;
  dataLen = pmap_node_->dataLen;

  // TODO
  // v0.1 range 从一开始就存 chunk ?
  // v0.2 sizeof(cur_) + sizeof(seq_)
  size_t offset = sizeof(size_t) << 1;
  while(offset < dataLen) {
    persistent_ptr<char[]> datChunkLen = pmap_node_->buf + offset + CHUNK_BLOOM_FILTER_LEN;
    size_t chunkLen = DecodeFixed64(datChunkLen.get());
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


void FixedRangeTab::GetRealRange(Slice &real_start, Slice &real_end)
{
  char *raw = pmap_node_->key_range_.get();
  real_start = GetKVData(raw, 0);
  real_end = GetKVData(raw, real_start.size() + sizeof(uint64_t));
}

Slice FixedRangeTab::GetKVData(char *raw, uint64_t item_off)
{
  char *target = raw + item_off;
  uint64_t target_size = DecodeFixed64(target);
  return Slice(target + sizeof(uint64_t), target_size);
}


Usage FixedRangeTab::RangeUsage()
{
  Usage usage;
  usage.range_size = pmap_node_->total_size_;
  usage.chunk_num = pmap_node_->chunk_num_;
  GetRealRange(usage.start, usage.end);
  return usage;
}

void FixedRangeTab::ConsistencyCheck() {
  uint64_t data_seq_num;
  data_seq_num = DecodeFixed64(raw_ - sizeof(uint64_t));
  p_range::Node* vnode = pmap_node_.get();
  if(data_seq_num != vnode->seq_num_){
    // TODO:又需要一个comparator
    /*Slice last_start, last_end;
        GetLastChunkKeyRange(last_start, last_end);*/
  }
}


void FixedRangeTab::Release()
{
  //TODO: release
  // 删除这个range
}

void FixedRangeTab::CleanUp()
{
  pmap_node_->dataLen = 0;
  RebuildBlkList();
}

} // namespace rocksdb
