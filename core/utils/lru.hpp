/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_LRU_HPP
#define KAGOME_UTILS_LRU_HPP

#include <boost/assert.hpp>
#include <unordered_map>

namespace kagome {
  /**
   * `std::unordered_map` with LRU.
   */
  template <typename K, typename V>
  class Lru {
   public:
    struct ItFwd;

    struct Item {
      V v;
      ItFwd more, less;
    };

    using Map = std::unordered_map<K, Item>;
    using It = typename Map::iterator;

    struct ItFwd : It {
      operator bool() const {
        return *this != It{};
      }
    };

    Lru(size_t capacity) : capacity_{capacity} {
      if (capacity_ == 0) {
        throw std::length_error{"Lru(capacity=0)"};
      }
    }

    Lru(const Lru &) = delete;
    void operator=(const Lru &) = delete;

    size_t capacity() const {
      return capacity_;
    }

    size_t size() const {
      return map_.size();
    }

    std::optional<std::reference_wrapper<V>> get(const K &k) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        return std::nullopt;
      }
      lru_use(it);
      return std::ref(it->second.v);
    }

    V &put(const K &k, V v) {
      return put2(k, std::move(v)).first->second.v;
    }

   private:
    std::pair<It, bool> put2(const K &k, V &&v) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        if (map_.size() >= capacity_) {
          lru_pop();
        }
        it = map_.emplace(k, Item{std::move(v), {}, {}}).first;
        lru_push(it);
        return {it, true};
      }
      it->second.v = std::move(v);
      lru_use(it);
      return {it, false};
    }

    void lru_use(It it) {
      if (it == most_) {
        return;
      }
      lru_extract(it->second);
      lru_push(it);
    }

    void lru_push(It it) {
      BOOST_ASSERT(not it->second.less);
      BOOST_ASSERT(not it->second.more);
      it->second.less = most_;
      if (most_) {
        most_->second.more = ItFwd{it};
      }
      most_ = ItFwd{it};
      if (not least_) {
        least_ = most_;
      }
    }

    void lru_extract(Item &v) {
      if (v.more) {
        v.more->second.less = v.less;
      } else {
        most_ = v.less;
      }
      if (v.less) {
        v.less->second.more = v.more;
      } else {
        least_ = v.more;
      }
      v.more = {};
      v.less = {};
    }

    void lru_pop() {
      auto it = least_;
      lru_extract(it->second);
      map_.erase(it);
    }

    Map map_;
    size_t capacity_;
    ItFwd most_, least_;

    template <typename>
    friend class LruSet;
  };

  template <typename K>
  class LruSet {
   public:
    LruSet(size_t capacity) : lru_{capacity} {}

    bool has(const K &k) {
      return lru_.get(k).has_value();
    }

    bool add(const K &k) {
      return lru_.put2(k, {}).second;
    }

   private:
    struct V {};

    Lru<K, V> lru_;
  };
}  // namespace kagome

#endif  // KAGOME_UTILS_LRU_HPP
