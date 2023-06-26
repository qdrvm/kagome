/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LRUCACHE
#define KAGOME_LRUCACHE

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <boost/assert.hpp>

#include "outcome/outcome.hpp"

namespace kagome {
  /**
   * LRU cache designed for small amounts of data (as its get() is O(N))
   */
  template <typename Key,
            typename Value,
            bool ValueMaybeShared = false,
            typename PriorityType = uint64_t>
  struct SmallLruCache final {
   public:
    static_assert(std::is_unsigned_v<PriorityType>);

    using CachedValue =
        std::conditional_t<ValueMaybeShared, std::shared_ptr<Value>, Value>;

    struct CacheEntry {
      Key key;
      CachedValue value;
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
          if constexpr (ValueMaybeShared) {
            auto &value = *entry.value;
            return value;
          } else {
            return entry.value;
          }
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

      if constexpr (ValueMaybeShared) {
        if constexpr (std::is_same_v<ValueArg, Value>) {
          auto it =
              std::find_if(cache_.begin(), cache_.end(), [&](const auto &item) {
                return *item.value == value;
              });
          if (it != cache_.end()) {
            auto &entry =
                cache_.emplace_back(CacheEntry{key, it->value, ticks_});
            return *entry.value;
          }
          auto &entry = cache_.emplace_back(
              CacheEntry{key,
                         std::make_shared<Value>(std::forward<ValueArg>(value)),
                         ticks_});
          return *entry.value;

        } else {
          CachedValue value_sptr =
              std::make_shared<Value>(std::forward<ValueArg>(value));
          auto it =
              std::find_if(cache_.begin(), cache_.end(), [&](const auto &item) {
                return *item.value == *value_sptr;
              });
          if (it != cache_.end()) {
            auto &entry =
                cache_.emplace_back(CacheEntry{key, it->value, ticks_});
            return *entry.value;
          }
          auto &entry = cache_.emplace_back(
              CacheEntry{key, std::move(value_sptr), ticks_});
          return *entry.value;
        }

      } else {
        auto &entry = cache_.emplace_back(
            CacheEntry{key, std::forward<ValueArg>(value), ticks_});
        return entry.value;
      }
    }

    outcome::result<std::reference_wrapper<const Value>> get_else(
        const Key &key, const std::function<outcome::result<Value>()> &func) {
      if (auto opt = get(key); opt.has_value()) {
        return opt.value();
      }
      if (auto res = func(); res.has_value()) {
        return put(key, std::move(res.value()));
      } else {
        return res.as_failure();
      }
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

  template <typename Key,
            typename Value,
            bool ValueMaybeShared = false,
            typename PriorityType = uint64_t>
  using LruCache = SmallLruCache<Key, Value, ValueMaybeShared, PriorityType>;

}  // namespace kagome

#endif  // KAGOME_LRUCACHE
