/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONTAINERS_OBJECTS_CACHE_HPP
#define KAGOME_CONTAINERS_OBJECTS_CACHE_HPP

#include <mutex>
#include <memory>
#include <type_traits>

namespace tools::containers {

  template <typename Type>
  struct ObjsCacheDefAlloc {
    template <typename... __args>
    Type *allocate(__args &&... args) {
      return new (std::nothrow) Type(std::forward<__args>(args)...);
    }
    void deallocate(Type *obj) {
      delete obj;
    }
  };

  template <typename T, typename Alloc = ObjsCacheDefAlloc<T>>
  struct ObjectsCache {
    static_assert(std::is_array<T>::value == false,
                  "array can not be used such way");

    using Type = T;
    using ObjectPtr = std::shared_ptr<Type>;

    ObjectsCache &operator=(const ObjectsCache &) = delete;
    ObjectsCache(const ObjectsCache &) = delete;

    ObjectsCache &operator=(ObjectsCache &&) = delete;
    ObjectsCache(ObjectsCache &&) = delete;

    ObjectsCache() = default;
    ObjectsCache(Alloc &alloc) : allocator_(alloc) {}

    virtual ~ObjectsCache() {
      for (auto *s : cache_) {
        allocator_.deallocate(s);
      }
    }

    Type *getCachedObject() {
      return getRawPtr();
    }

    void setCachedObject(Type *const ptr) {
      setRawPtr(ptr);
    }

    ObjectPtr getSharedCachedObject() {
      return ObjectPtr(getRawPtr(), [&](auto *obj) mutable { setRawPtr(obj); });
    }

   private:
    using ObjectsArray = std::vector<Type *>;

    Alloc allocator_;
    std::mutex cache_blocker_;
    ObjectsArray cache_;

    inline Type *getRawPtr() {
      Type *ptr;
      std::lock_guard guard(cache_blocker_);
      if (!cache_.empty()) {
        ptr = cache_.back();
        cache_.pop_back();
      } else {
        ptr = allocator_.allocate();
      }
      return ptr;
    }

    inline void setRawPtr(Type *const ptr) {
      if (nullptr != ptr) {
        std::lock_guard guard(cache_blocker_);
        cache_.push_back(ptr);
      }
    }
  };

  template <typename T>
  struct CacheUnit {
    using Type =
        typename std::remove_pointer<typename std::decay<T>::type>::type;
  };

  template <typename T, typename... ARGS>
  struct ObjectsCacheManager : public ObjectsCache<typename T::Type>,
                               public ObjectsCacheManager<ARGS...> {};

  template <typename T>
  struct ObjectsCacheManager<T> : public ObjectsCache<typename T::Type> {};

#ifndef KAGOME_CACHE_UNIT
#define KAGOME_CACHE_UNIT(type) tools::containers::CacheUnit<type>
#endif  // KAGOME_CACHE_UNIT

#ifndef KAGOME_DECLARE_CACHE
#define KAGOME_DECLARE_CACHE(prefix, ...) \
        using prefix##_cache_type = tools::containers::ObjectsCacheManager<__VA_ARGS__>; \
        extern prefix##_cache_type prefix##_cache; \
        template <typename T> inline T *prefix##_get_from_cache() { \
            return static_cast<tools::containers::ObjectsCache<T> *>(&prefix##_cache)->getCachedObject(); \
        } \
        template <typename T> inline void prefix##_set_to_cache(T *const ptr) { \
            static_cast<tools::containers::ObjectsCache<T> *>(&prefix##_cache)->setCachedObject(ptr); \
        } \
        template <typename T> inline std::shared_ptr<T> prefix##_get_shared_from_cache() { \
            return static_cast<tools::containers::ObjectsCache<T> *>(&prefix##_cache)->getSharedCachedObject(); \
        } \
        template <typename T> inline std::unique_ptr<T, void (*)(T *const)> prefix##_get_unique_from_cache() { \
            return std::unique_ptr<T, void (*)(T *const)>(prefix##_get_from_cache<T>(), &prefix##_set_to_cache<T>); \
        }
#endif  // KAGOME_DECLARE_CACHE

#ifndef KAGOME_DEFINE_CACHE
#define KAGOME_DEFINE_CACHE(prefix) prefix##_cache_type prefix##_cache;
#endif  // KAGOME_DEFINE_CACHE

#ifndef KAGOME_EXTRACT_SHARED_CACHE
#define KAGOME_EXTRACT_SHARED_CACHE(prefix, type) prefix##_get_shared_from_cache<type>()
#endif  // KAGOME_EXTRACT_SHARED_CACHE

#ifndef KAGOME_EXTRACT_UNIQUE_CACHE
#define KAGOME_EXTRACT_UNIQUE_CACHE(prefix, type) prefix##_get_unique_from_cache<type>()
#endif  // KAGOME_EXTRACT_UNIQUE_CACHE

#ifndef KAGOME_EXTRACT_RAW_CACHE
#define KAGOME_EXTRACT_RAW_CACHE(prefix, type) prefix##_get_from_cache<type>()
#endif  // KAGOME_EXTRACT_RAW_CACHE

#ifndef KAGOME_INSERT_RAW_CACHE
#define KAGOME_INSERT_RAW_CACHE(prefix, obj) prefix##_set_to_cache(obj)
#endif  // KAGOME_INSERT_RAW_CACHE

}

#endif//KAGOME_CONTAINERS_OBJECTS_CACHE_HPP