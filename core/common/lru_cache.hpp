/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LRUCACHE
#define KAGOME_LRUCACHE

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include <boost/assert.hpp>

namespace kagome {
  /**
   * LRU cache designed for small amounts of data (as its get() is O(N))
   */
  template <typename Key, typename Value, typename PriorityType = uint64_t>
  struct SmallLruCache final {
   public:
    static_assert(std::is_unsigned_v<PriorityType>);

    struct CacheEntry {
      Key key;
      Value value;
      PriorityType latest_use_tick_;

      bool operator<(const CacheEntry &rhs) const {
        return latest_use_tick_ < rhs.latest_use_tick_;
      }
    };

    SmallLruCache(size_t max_size) : kMaxSize{max_size} {
      BOOST_ASSERT(kMaxSize > 0);
      cache_.reserve(kMaxSize);
    }

    std::optional<std::reference_wrapper<const Value>> get(const Key &key) {
      ticks_++;
      if (ticks_ == 0) {
        handleTicksOverflow();
      }
      for (auto &entry : cache_) {
        if (entry.key == key) {
          entry.latest_use_tick_ = ticks_;
          return entry.value;
        }
      }
      return std::nullopt;
    }

    template <typename ValueArg>
    std::reference_wrapper<const Value> put(const Key &key, ValueArg &&value) {
      static_assert(std::is_convertible_v<ValueArg, Value>
                    || std::is_constructible_v<ValueArg, Value>);
      ticks_++;
      if (cache_.size() >= kMaxSize) {
        auto min = std::min_element(cache_.begin(), cache_.end());
        cache_.erase(min);
      }
      auto &entry = cache_.emplace_back(
          CacheEntry{key, std::forward<ValueArg>(value), ticks_});
      return entry.value;
    }

   private:
    void handleTicksOverflow() {
      // 'compress' timestamps of entries in the cache (works because we care
      // only about their order, not actual timestamps)
      std::sort(cache_.begin(), cache_.end());
      for (auto &entry : cache_) {
        entry.latest_use_tick_ = ticks_;
        ticks_++;
      }
    }

    const size_t kMaxSize;
    // an abstract representation of time to implement 'recency' without
    // depending on real time. Incremented on each cache access
    PriorityType ticks_{};
    std::vector<CacheEntry> cache_;
  };

  template <typename... T>
  using LruCache = SmallLruCache<T...>;

}  // namespace kagome

#endif  // KAGOME_LRUCACHE
