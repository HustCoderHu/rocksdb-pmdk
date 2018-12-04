#include "pmem_hash_map.h"
#include <string>
#include <vector>
#include <iostream>

#include <unistd.h> // F_OK

using std::cout;
using std::endl;
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

#define CREATE_MODE_RW (S_IWRITE | S_IREAD)

static inline int
file_exists(char const *file) {
  return access(file, F_OK);
}

using p_range::pmem_hash_map;

class Content {
public:
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
  uint64_t hashCode()
  {
    uint64_t v = 0;
    for (size_t i = 0; i < bufSize_; ++i) {
      v += prefix_[i];
    }
    return v;
  }

  p_buf prefix_;
  p<size_t> prefixLen_;
  p<size_t> bufSize_;
};


int main(int argc, char* argv[])
{
  string path = "/mnt/pmem/test_pmem_hash_map.pmem";
  pool<pmem_hash_map<Content> > pop;
  persistent_ptr<pmem_hash_map<Content> > p_map;

  if (file_exists(path.c_str()) != 0) {
    cout << "create pool" << endl;
    pop = pool<pmem_hash_map<Content> >::create(path.c_str(), "layout",PMEMOBJ_MIN_POOL,
                                                CREATE_MODE_RW);
    p_map = pop.root();
    p_map->init(pop, 0.75f, 32);
  } else {
    cout << "open pool" << endl;
    pop = pool<pmem_hash_map<Content> >::open(path.c_str(), "layout");
  }
  //
  p_map = pop.root();

  persistent_ptr<Content> p_content = nullptr;

  // 至少一个参数，就 put
  if (argc > 1) {
    for (int i = 0; i < 13; ++i) {
      int val = 2 << i;
      transaction::run(pop, [&] {
        MY_PRINT("\n");
        string str = std::to_string(val);
        cout << "val = " << str.c_str() << endl;
        p_content = make_persistent<Content>(pop, str, val);
        MY_PRINT("\n");
      });
//      p_content = make_persistent<Content>(pop, std::to_string(val), val);
      p_map->put(pop, p_content);
    }
  }
  else { // 否则 输出存储的内容
    vector<persistent_ptr<Content> > contentVec;

    p_map->getAll(contentVec);

    printf("vector len = %zu\n", contentVec.size());

    size_t i = 0;
    for (persistent_ptr<Content> cont : contentVec) {
      printf("%zu\n", i++);
      cout << "prefixLen_ = " << cont->prefixLen_ << endl;
      cout << "bufSize_ = " << cont->bufSize_ << endl;
      cout << "char prefix_ = ";
      for (size_t j = 0; j < cont->prefixLen_; ++j) {
        putchar(cont->prefix_[j]);
      }
      putchar('\n');

      string str;
      str.assign(cont->prefix_.get(), cont->prefixLen_);
      cout << "str prefix_ = " << str << endl;
      putchar('\n');
      size_t bufSize_ = cont->bufSize_;
  }
  pop.close();
  return 0;
}
