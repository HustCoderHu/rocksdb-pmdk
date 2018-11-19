#include "fixed_range_tab.h"
#include <persistent_chunk.h>

#include <table/merging_iterator.h>
#include <libpmemobj++/transaction.hpp>

namespace rocksdb {

using pmem::obj::persistent_ptr;
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



FixedRangeTab::FixedRangeTab(pool<p_range::pmem_hash_map> *pop, p_range::p_node node_in_pmem_map)
  : chunk_sum_size(0)
  , MAX_CHUNK_SUM_SIZE(64 * 1024 * 1024)
  , pop_(pop)
  , node_in_pmem_map_(node_in_pmem_map)
{
  buildBlkList();
}

FixedRangeTab::~FixedRangeTab()
{

}

//| used_bits ... | 预设 start | 预设 end |


//| chunk blmFilter | chunk ...   |  不定长
//| chunk blmFilter | chunk ...  |  不定长
//| chunk blmFilter | chunk ...    |  不定长
//| real_start | real_end |


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

Status FixedRangeTab::Get(const Slice &key, std::string *value)
{
  // TODO
  for (ChunkBlk &blk : blklist) {
    persistent_ptr<char[]> datFilter = node_in_pmem_map_->buf + blk.offset_;

    // check bloom

    size_t chunkLen = blk.chunkLen_;
    persistent_ptr<char[]> exactChunk = node_in_pmem_map_->buf + blk.getDatOffset();
  }

  // return ?;
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
  TX_BEGIN(pop) {
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
  } TX_END
}

void FixedRange::Release()
{

}

} // namespace rocksdb
