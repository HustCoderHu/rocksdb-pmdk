#include "pmem_hash_map.h"
#include <string>
#include <vector>

using std::string;
using std::to_string;
using std::vector;

using pmem::obj::pool_base;
using pmem::obj::pool;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;
using pmem::obj::transaction;
using p_buf = persistent_ptr<char[]>;

using namespace p_range;

static inline int
file_exists(char const *file) {
  return access(file, F_OK);
}

class Content {
  Content(pool_base &pop, const string &prefix, size_t range_size)
  {
    transaction::run(pop, [&] {
      bufSize_ = range_size;
      //      dataLen_ = 0;
      prefixLen_ = prefix.length();
      prefix_ = make_persistent<char[]>(prefixLen_);
      memcpy(prefix_.get(), prefix.data(), prefixLen_);
    });
  }

  uint64_t hashCode(const string& prefix)
  {
    uint64_t v = 0;
    for (char c : prefix)
      v += c;
    return v;
  }

  p_buf prefix_;
  p<size_t> prefixLen_;
  p<size_t> bufSize_;
};


int main(int argc, char* argv[])
{
  string path = "";
  pool<pmem_hash_map<Content> > pop;

  if (file_exists(path.c_str())) {
    pop = pool<pmem_hash_map<Content> >::open(path.c_str(), "layout";
  } else
    pop = pool<pmem_hash_map<Content> >::create(path.c_str(), "layout",PMEMOBJ_MIN_POOL,
                                                CREATE_MODE_RW);
  //
  persistent_ptr<pmem_hash_map<Content> > p_map = pop.root();

  persistent_ptr<Content> p_content = nullptr;

  // 至少一个参数，就 put
  if (argc > 1) {

    for (int i = 0; i < 13; ++i) {
      int val = 2 << i;
      p_content = make_persistent<Content>(pop, std::to_string(val), val);
      p_map->put(pop, p_content);
    }
  }
  else { // 否则 输出存储的内容
    vector<persistent_ptr<Content> > contentVec;

    p_map->getAll(contentVec);

    printf("vector len = %zu\n", contentVec.size());

    size_t i = 0;
    for (persistent_ptr<Content> cont : contentVec) {
      string str(cont->prefix_.get(), cont->prefixLen_);
      size_t bufSize_ = cont->bufSize_;
      printf("%zu\n", i++);
      printf("prefix_ = %s\n", str);
      printf("bufSize_ = %zu\n", bufSize_);
    }
  }
}
