/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_LRU_HPP
#define KAGOME_UTILS_LRU_HPP

#include <boost/assert.hpp>
#include <type_traits>
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
      template <typename... Args>
      Item(Args &&...args)
          : v{std::forward<Args>(args)...}, more{}, less{} {}

      V v;
      ItFwd more, less;
    };

    using Map = std::unordered_map<K, Item>;
    using It = typename Map::iterator;
    using ConstIt = typename Map::const_iterator;

    struct ItFwd : It {
      operator bool() const {
        return *this != It{};
      }
    };

    enum class IteratorQualifier {
      Const,
      Mutable,
    };

    template <IteratorQualifier Qualifier>
    class Iterator {
      template <IteratorQualifier>
      friend class Iterator;

     public:
      using WrappedIt = typename std::
          conditional<Qualifier == IteratorQualifier::Const, ConstIt, It>::type;

      explicit Iterator(WrappedIt it) : iter_{it} {}

      // mutable version
      template <IteratorQualifier _Qualifier = Qualifier>
      std::enable_if_t<_Qualifier == IteratorQualifier::Mutable, V &> value() {
        return iter_->second.v;
      }

      const V &value() const {
        return iter_->second.v;
      }

      Iterator &operator++() {
        iter_++;
      }

      template <IteratorQualifier OtherQualifier>
      bool operator==(const Iterator<OtherQualifier> &other) const {
        return iter_ == other.iter_;
      }

      template <IteratorQualifier OtherQualifier>
      bool operator!=(const Iterator<OtherQualifier> &other) const {
        return !(*this == other);
      }

     private:
      WrappedIt iter_;
    };

    using ConstIterator = Iterator<IteratorQualifier::Const>;
    using MutIterator = Iterator<IteratorQualifier::Mutable>;

    explicit Lru(size_t capacity) : capacity_{capacity} {
      if (capacity_ == 0) {
        throw std::length_error{"Zero capacity is invalid for Lru"};
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

    ConstIterator begin() const {
      return ConstIterator{map_.begin()};
    }

    ConstIterator end() const {
      return ConstIterator{map_.end()};
    }

    MutIterator find(const K &k) {
      return MutIterator{map_.find(k)};
    }

    ConstIterator find(const K &k) const {
      return ConstIterator{map_.find(k)};
    }

    std::pair<MutIterator, bool> put(const K &k, V v) {
      auto [it, is_put] = put2(k, std::move(v)).first->second.v;
      return std::make_pair(it, is_put);
    }

    template <typename... ValueArgs>
    std::pair<MutIterator, bool> emplace(const K &k, ValueArgs &&...args) {
      auto [it, is_emplaced] = emplace2(k, std::forward<ValueArgs>(args)...);
      return std::make_pair(MutIterator{it}, is_emplaced);
    }

    typename Map::node_type extract(const K &key) {
      auto it = map_.find(key);
      if (it == map_.end()) {
        return typename Map::node_type{};
      }
      auto handle = map_.extract(it);
      lru_process_remove(it->second);
      return handle;
    }

   private:
    template <typename... ValueArgs>
    std::pair<It, bool> emplace2(const K &k, ValueArgs &&...args) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        if (map_.size() >= capacity_) {
          lru_pop();
        }
        it = map_.emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(k),
                     std::forward_as_tuple(
                         std::forward<ValueArgs>(args)...))
                 .first;
        lru_push(it);
        return {it, true};
      }
      return {it, false};
    }

    std::pair<It, bool> put2(const K &k, V &&v) {
      auto it = map_.find(k);
      if (it == map_.end()) {
        if (map_.size() >= capacity_) {
          lru_pop();
        }
        it = map_.emplace(std::piecewise_construct,
                          std::forward_as_tuple(k),
                          std::forward_as_tuple(std::move(v)))
                 .first;
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
      lru_process_remove(it->second);
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

    void lru_process_remove(Item &v) {
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
      lru_process_remove(it->second);
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
