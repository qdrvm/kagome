/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <boost/assert.hpp>

#include "outcome/outcome.hpp"
#include "utils/retain_if.hpp"

namespace kagome {

  template <template <bool> class Lockable, bool IsLockable>
  struct LockGuard {
    inline LockGuard(const Lockable<IsLockable> &) {}
  };

  template <template <bool> class Lockable>
  struct LockGuard<Lockable, true> {
    LockGuard(const Lockable<true> &protected_object)
        : protected_object_(protected_object) {
      protected_object_.lock();
    }
    ~LockGuard() {
      protected_object_.unlock();
    }

   private:
    const Lockable<true> &protected_object_;
  };

  template <bool IsLockable>
  class Lockable {
   protected:
    friend struct LockGuard<Lockable, IsLockable>;
    inline void lock() const {}
    inline void unlock() const {}
  };

  template <>
  class Lockable<true> {
   protected:
    friend struct LockGuard<Lockable, true>;
    inline void lock() const {
      mutex_.lock();
    }
    inline void unlock() const {
      mutex_.unlock();
    }
    mutable std::mutex mutex_;
  };

  /**
   * LRU cache designed for small amounts of data (as its get() is O(N))
   */
  template <typename Key,
            typename Value,
            bool ThreadSafe = false,
            typename PriorityType = uint64_t>
  struct SmallLruCache final : public Lockable<ThreadSafe> {
   public:
    static_assert(std::is_unsigned_v<PriorityType>);

    using Mutex = Lockable<ThreadSafe>;

    struct CacheEntry {
      Key key;
      std::shared_ptr<Value> value;
      PriorityType latest_use_tick_;

      bool operator<(const CacheEntry &rhs) const {
        return latest_use_tick_ < rhs.latest_use_tick_;
      }
    };

    SmallLruCache(size_t max_size) : kMaxSize{max_size} {
      BOOST_ASSERT(kMaxSize > 0);
      cache_.reserve(kMaxSize);
    }

    std::optional<std::shared_ptr<const Value>> get(const Key &key) {
      LockGuard lg(*this);
      if (++ticks_ == 0) {
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
    std::shared_ptr<const Value> put(const Key &key, ValueArg &&value) {
      LockGuard lg(*this);
      static_assert(std::is_convertible_v<ValueArg, Value>
                    || std::is_constructible_v<ValueArg, Value>);
      if (cache_.size() >= kMaxSize) {
        auto min = std::min_element(cache_.begin(), cache_.end());
        cache_.erase(min);
      }

      if (++ticks_ == 0) {
        handleTicksOverflow();
      }

      if constexpr (std::is_same_v<ValueArg, Value>) {
        auto it =
            std::find_if(cache_.begin(), cache_.end(), [&](const auto &item) {
              return *item.value == value;
            });
        if (it != cache_.end()) {
          auto &entry = cache_.emplace_back(CacheEntry{key, it->value, ticks_});
          return entry.value;
        }
        auto &entry = cache_.emplace_back(
            CacheEntry{key,
                       std::make_shared<Value>(std::forward<ValueArg>(value)),
                       ticks_});
        return entry.value;

      } else {
        auto value_sptr =
            std::make_shared<Value>(std::forward<ValueArg>(value));
        auto it =
            std::find_if(cache_.begin(), cache_.end(), [&](const auto &item) {
              return *item.value == *value_sptr;
            });
        if (it != cache_.end()) {
          auto &entry = cache_.emplace_back(CacheEntry{key, it->value, ticks_});
          return entry.value;
        }
        auto &entry =
            cache_.emplace_back(CacheEntry{key, std::move(value_sptr), ticks_});
        return entry.value;
      }
    }

    outcome::result<std::shared_ptr<const Value>> get_else(
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

    void erase(const Key &key) {
      LockGuard lg(*this);
      auto it = std::find_if(cache_.begin(),
                             cache_.end(),
                             [&](const auto &item) { return item.key == key; });
      if (it != cache_.end()) {
        cache_.erase(it);
      }
    }

    void erase_if(const std::function<bool(const Key &key, const Value &value)>
                      &predicate) {
      LockGuard lg(*this);
      retain_if(cache_, [&](const CacheEntry &item) {
        return not predicate(item.key, *item.value);
      });
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
            bool ThreadSafe = true,
            typename PriorityType = uint64_t>
  using LruCache = SmallLruCache<Key, Value, ThreadSafe, PriorityType>;

}  // namespace kagome
