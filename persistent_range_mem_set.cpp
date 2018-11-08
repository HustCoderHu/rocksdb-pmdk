#include "persistent_range_mem_set.h"

#include <ex_common.h>

namespace rocksdb {

PersistentRangeMemSet::PersistentRangeMemSet()
{
  //    file_path.data()
  if (file_exists(path) != 0) {
    if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(range_mem),
                              PMEMOBJ_MIN_POOL, 0666)) == NULL) {
      perror("failed to create pool\n");
      return 1;
    }
  } else {
    if ((pop = pmemobj_open(path,
                            POBJ_LAYOUT_NAME(range_mem))) == NULL) {
      perror("failed to open pool\n");
      return 1;
    }
  }
}

PersistentRangeMemSet::~PersistentRangeMemSet()
{
  if (pop)
    pmemobj_close(pop);
}

} // namespace rocksdb


