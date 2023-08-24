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
    struct Item;
    // TODO(turuslan): remove unique_ptr after deprecating gcc 10
    using Map = std::unordered_map<K, std::unique_ptr<Item>>;
    using It = typename Map::iterator;
    struct Item {
      V v;
      It more, less;
    };

    explicit Lru(size_t capacity) : capacity_{capacity} {
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
      return std::ref(it->second->v);
    }

    V &put(const K &k, V v) {
      return put2(k, std::move(v)).first->second->v;
    }

    void erase(const K &k) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        return;
      }
      lru_extract(*it->second);
      map_.erase(it);
    }

    template <typename F>
    void erase_if(const F &f) {
      for (auto it = map_.begin(); it != map_.end();) {
        if (f(it->first, it->second->v)) {
          ++it;
        } else {
          lru_extract(*it->second);
          it = map_.erase(it);
        }
      }
    }

   private:
    static auto empty(const It &it) {
      return it == It{};
    }

    std::pair<It, bool> put2(const K &k, V &&v) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        if (map_.size() >= capacity_) {
          lru_pop();
        }
        it = map_.emplace(k, std::make_unique<Item>(Item{std::move(v), {}, {}}))
                 .first;
        lru_push(it);
        return {it, true};
      }
      it->second->v = std::move(v);
      lru_use(it);
      return {it, false};
    }

    void lru_use(It it) {
      if (it == most_) {
        return;
      }
      lru_extract(*it->second);
      lru_push(it);
    }

    void lru_push(It it) {
      BOOST_ASSERT(empty(it->second->less));
      BOOST_ASSERT(empty(it->second->more));
      it->second->less = most_;
      if (not empty(most_)) {
        most_->second->more = it;
      }
      most_ = it;
      if (empty(least_)) {
        least_ = most_;
      }
    }

    void lru_extract(Item &v) {
      if (not empty(v.more)) {
        v.more->second->less = v.less;
      } else {
        most_ = v.less;
      }
      if (not empty(v.less)) {
        v.less->second->more = v.more;
      } else {
        least_ = v.more;
      }
      v.more = It{};
      v.less = It{};
    }

    void lru_pop() {
      auto it = least_;
      lru_extract(*it->second);
      map_.erase(it);
    }

    Map map_;
    size_t capacity_;
    It most_, least_;

    template <typename>
    friend class LruSet;
  };

  template <typename K>
  class LruSet {
   public:
    explicit LruSet(size_t capacity) : lru_{capacity} {}

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
