#include "persistent_range_mem.h"

#include <ex_common.h>


#include <table/merging_iterator.h>

#include <persistent_chunk.h>

#define MAX_BUF_LEN 4096


POBJ_LAYOUT_BEGIN(two_lists);
POBJ_LAYOUT_ROOT(two_lists, struct my_root);
POBJ_LAYOUT_TOID(two_lists, struct foo_el);
POBJ_LAYOUT_TOID(two_lists, struct bar_el);
POBJ_LAYOUT_END(two_lists);

struct my_root {
    size_t length;
    char name[MAX_BUF_LEN];
};

rocksdb::PersistentRangeMem::PersistentRangeMem()
{
    //    file_path.data()
    if (file_exists(path) != 0) {
        if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(two_lists),
                                  PMEMOBJ_MIN_POOL, 0666)) == NULL) {
            perror("failed to create pool\n");
            return 1;
        }
    } else {
        if ((pop = pmemobj_open(path,
                                POBJ_LAYOUT_NAME(two_lists))) == NULL) {
            perror("failed to open pool\n");
            return 1;
        }
    }
}

rocksdb::InternalIterator *rocksdb::PersistentRangeMem::NewInternalIterator(
        ColumnFamilyData *cfd, Arena *arena)
{
    InternalIterator* internal_iter;

    MergeIteratorBuilder merge_iter_builder(&cfd->internal_comparator(),
                                            arena);

    for (PersistentChunk &psttChunk : psttChunkList) {
        merge_iter_builder.AddIterator(psttChunk.NewInternalIterator());
    }

    internal_iter = merge_iter_builder.Finish();
}

void rocksdb::PersistentRangeMem::Append(const char *bloom_data,
                                         const Slice &chunk_data)
{
    if (chunk_sum_size + chunk_data.size() >= MAX_CHUNK_SUM_SIZE) {
        // flush
    }

    TX_BEGIN(pop) {
        /* TX_STAGE_WORK */
    } TX_ONCOMMIT {
        /* TX_STAGE_ONCOMMIT */
    } TX_ONABORT {
        /* TX_STAGE_ONABORT */
    } TX_FINALLY {
        /* TX_STAGE_FINALLY */
    } TX_END

            chunk_sum_size += chunk_data.size();
}
