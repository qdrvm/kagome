#define _GNU_SOURCE
#include "utils/memory_profiler.hpp"

#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <assert.h>
#include <malloc.h>
#include <vector>
#include <algorithm>
#include <optional>
#include <execinfo.h>
#include <cstring>

/*=========================================================
 * interception points
 */

typedef void * (*Myfn_calloc)(size_t, size_t);
typedef void * (*Myfn_malloc)(size_t );
typedef void   (*Myfn_free)(void *);
typedef void * (*Myfn_realloc)(void *, size_t );
typedef void * (*Myfn_memalign)(size_t , size_t );

static Myfn_calloc   myfn_calloc;
static Myfn_malloc   myfn_malloc;
static Myfn_free     myfn_free;
static Myfn_realloc  myfn_realloc;
static Myfn_memalign myfn_memalign;

namespace {
  template <typename T>
  class mmap_allocator : public std::allocator<T> {
   public:
    typedef size_t size_type;
    typedef T *pointer;
    typedef const T *const_pointer;

    template <typename _Tp1>
    struct rebind {
      typedef mmap_allocator<_Tp1> other;
    };

    pointer allocate(size_type n, const void *hint = 0) {
      assert(myfn_malloc != nullptr);
      return (pointer)myfn_malloc(n * sizeof(T));
    }

    void deallocate(pointer p, size_type n) {
      assert(myfn_free != nullptr);
      return myfn_free(p);
    }

    mmap_allocator() throw() : std::allocator<T>() {}
    mmap_allocator(const mmap_allocator &a) throw() : std::allocator<T>(a) {}
    template <class U>
    mmap_allocator(const mmap_allocator<U> &a) throw() : std::allocator<T>(a) {}
    ~mmap_allocator() throw() {}
  };

  static constexpr size_t kBufferSize = 1024ull * 1024ull;
  static constexpr size_t kStackSize = 20;

  struct StackData final {
    void *data[kStackSize];
    size_t count;

    StackData() : count(0ull) {}
    StackData(StackData const &) = delete;
    StackData(StackData &&) = delete;
  };

  struct AllocationDescription final {
    uint64_t count;
    uint64_t alloc_size;
    std::optional<StackData> stack;

    AllocationDescription(AllocationDescription const &) = delete;
    AllocationDescription(AllocationDescription &&) = delete;
    AllocationDescription &operator=(AllocationDescription const &) = delete;
    AllocationDescription &operator=(AllocationDescription &&) = delete;
    AllocationDescription() : count(0ull), alloc_size(0ull) {}
  };

  using HashTableType = size_t;
  using AllocationsTableType = std::unordered_map<
      HashTableType,
      AllocationDescription,
      std::hash<HashTableType>,
      std::equal_to<HashTableType>,
      mmap_allocator<std::pair<const HashTableType, AllocationDescription>>>;
  using PointersTableType = std::unordered_map<
      uintptr_t,
      HashTableType,
      std::hash<uintptr_t>,
      std::equal_to<uintptr_t>,
      mmap_allocator<std::pair<const uintptr_t, HashTableType>>>;

  char allocations_table[sizeof(AllocationsTableType)];
  AllocationsTableType &allocationsTable() {
    return *(AllocationsTableType *)allocations_table;
  }

  char pointers_table[sizeof(PointersTableType)];
  PointersTableType &pointersTable() {
    return *(PointersTableType *)pointers_table;
  }

  std::recursive_mutex tables_cs;
  std::atomic<bool> table_ready = false;

  char track[kBufferSize + 1];
  void makeDelete(void *ptr) {
    if (table_ready.load()) {
      std::lock_guard lock(tables_cs);
      if (auto it = pointersTable().find(uintptr_t(ptr));
          it != pointersTable().end()) {
        auto const hash = it->second;
        pointersTable().erase(it);

        auto it_hash = allocationsTable().find(hash);
        assert(it_hash != allocationsTable().end());

        it_hash->second.alloc_size -= malloc_usable_size(ptr);
        if (--it_hash->second.count == 0ull)
          allocationsTable().erase(it_hash);
      }
    }
  }

  void registerAllocation(void *ptr) {
    uint64_t total_allocated = 0ull;
    if (table_ready.load()) {
      std::lock_guard lock(tables_cs);
      static bool skip_profile = false;
      if (skip_profile)
        return;

      void *a[kStackSize];
      skip_profile = true; // iceseer: to skip recursive call
      auto const s = backtrace(a, kStackSize);
      skip_profile = false;

      auto const hash = std::_Hash_impl::hash(a, s * sizeof(a[0]));
      auto &entry = allocationsTable()[hash];
      if (!entry.stack) {
        entry.stack.emplace();
        std::memcpy(entry.stack->data, a, s * sizeof(a[0]));
        entry.stack->count = s;
      }
      ++entry.count;
      entry.alloc_size += malloc_usable_size(ptr);
      total_allocated = entry.alloc_size;
      pointersTable()[uintptr_t(ptr)] = hash;
    }

    if (total_allocated >= 1*1024ull*1024ull*1024ull) {
      profiler::deinitTables();
      profiler::printTables("./allocations.log");
      abort();
    }
  }
}

namespace profiler {
  void initTables() {
    std::lock_guard lock(tables_cs);
    new (allocations_table) AllocationsTableType();
    new (pointers_table) PointersTableType();
    table_ready.store(true);
  }

  void deinitTables() {
    std::lock_guard lock(tables_cs);
    table_ready.store(false);
  }

  void printTables(char const *filename) {
    std::lock_guard lock(tables_cs);
    std::vector<AllocationDescription *,
                mmap_allocator<AllocationDescription *>>
        descriptors;
    descriptors.reserve(allocationsTable().size());
    for (auto &it : allocationsTable()) descriptors.push_back(&it.second);

    std::sort(descriptors.begin(), descriptors.end(), [](auto a, auto b) {
      return a->alloc_size > b->alloc_size;
    });

    std::ofstream output(filename, std::ios::out | std::ios::trunc);
    output << "[MEMORY PROFILER]\n";
    for (auto &item : descriptors) {
      auto pos = snprintf(track,
                          kBufferSize,
                          "<TRACE> count: %llu, allocated: %llu\n",
                          (long long unsigned int)item->count,
                          (long long unsigned int)item->alloc_size);

      assert(item->stack);
      char **names = backtrace_symbols(item->stack->data, item->stack->count);
      for (size_t ix = 0; ix < item->stack->count; ++ix)
        pos += snprintf(track + pos, kBufferSize - pos, "%s\n", names[ix]);

      free(names);
      output << track;
    }
    output.flush();
  }
}

char tmpbuff[1024];
unsigned long tmppos = 0;
unsigned long tmpallocs = 0;

void *memset(void*,int,size_t);
void *memmove(void *to, const void *from, size_t size);

static void init()
{
  myfn_malloc     = (Myfn_malloc)dlsym(RTLD_NEXT, "malloc");
  myfn_free       = (Myfn_free)dlsym(RTLD_NEXT, "free");
  myfn_calloc     = (Myfn_calloc)dlsym(RTLD_NEXT, "calloc");
  myfn_realloc    = (Myfn_realloc)dlsym(RTLD_NEXT, "realloc");
  myfn_memalign   = (Myfn_memalign)dlsym(RTLD_NEXT, "memalign");

  if (!myfn_malloc || !myfn_free || !myfn_calloc || !myfn_realloc || !myfn_memalign)
  {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    exit(1);
  }
}

void *malloc(size_t size) {
  static std::atomic<int> initializing = 0;
  if (myfn_malloc == NULL) {
    int expected = 0;
    if (initializing.compare_exchange_strong(expected, 1)) {
      init();
      initializing = 0;
      fprintf(stdout,
              "jcheck: allocated %lu bytes of temp memory in %lu chunks during initialization\n",
              tmppos,
              tmpallocs);
    } else {
      if (tmppos + size < sizeof(tmpbuff)) {
        void *retptr = tmpbuff + tmppos;
        tmppos += size;
        ++tmpallocs;
        return retptr;
      } else {
        fprintf(stdout,
                "jcheck: too much memory requested during initialisation - increase tmpbuff size\n");
        exit(1);
      }
    }
  }

  void *ptr = myfn_malloc(size);
  registerAllocation(ptr);
  return ptr;
}

void free(void *ptr)
{
  if (ptr >= (void*) tmpbuff && ptr <= (void*)(tmpbuff + tmppos))
    fprintf(stdout, "freeing temp memory\n");
  else {
    makeDelete(ptr);
    myfn_free(ptr);
  }
}

void *realloc(void *ptr, size_t size) {
  if (myfn_malloc == NULL) {
    void *nptr = malloc(size);
    if (nptr && ptr) {
      memmove(nptr, ptr, size);
      free(ptr);
    }
    return nptr;
  }

  void *nptr = myfn_realloc(ptr, size);
  makeDelete(ptr);
  registerAllocation(nptr);

  return nptr;
}

void *calloc(size_t nmemb, size_t size)
{
  if (myfn_malloc == NULL)
  {
    void *ptr = malloc(nmemb*size);
    if (ptr)
      memset(ptr, 0, nmemb*size);
    return ptr;
  }

  void *ptr = myfn_calloc(nmemb, size);
  registerAllocation(ptr);
  return ptr;
}

void *memalign(size_t blocksize, size_t bytes)
{
  void *ptr = myfn_memalign(blocksize, bytes);
  registerAllocation(ptr);
  return ptr;
}
