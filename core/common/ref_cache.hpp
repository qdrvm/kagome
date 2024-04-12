/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <memory>
#include <optional>

namespace kagome {

  /**
   * @brief Cache keeps an item inside if any reference exists. If all
   * references lost, then item removes from cache. If an item exists, then it
   * returns new reference on it.
   */
  template <typename K, typename V>
  struct RefCache : std::enable_shared_from_this<RefCache<K, V>> {
    struct RefObj;

    using Value = std::decay_t<V>;
    using Container = std::map<K, std::weak_ptr<RefObj>>;
    using Iterator = typename Container::iterator;
    using SRefCache = std::shared_ptr<RefCache>;
    using Key = K;

    struct RefObj {
      template <typename... Args>
      RefObj(Args &&...args) : obj_(std::forward<Args>(args)...) {}

      RefObj(RefObj &&) = default;
      RefObj(const RefObj &) = delete;

      RefObj &operator=(RefObj &&) = delete;
      RefObj &operator=(const RefObj &) = delete;

      ~RefObj() {
        if (keeper_) {
          keeper_->remove(it_);
        }
      }

      Value &value_mut() {
        return obj_;
      }

      const Value &value() const {
        return obj_;
      }

     private:
      void store_internal_refs(Iterator it, SRefCache keeper) {
        BOOST_ASSERT(keeper);
        BOOST_ASSERT(!keeper_);

        it_ = it;
        keeper_ = keeper;
      }

      friend struct RefCache;
      Value obj_;
      Iterator it_;
      SRefCache keeper_;
    };
    using SRefObj = std::shared_ptr<RefObj>;

    static SRefCache create() {
      return SRefCache(new RefCache());
    }

    /**
     * @brief returns an element, if exists. Otherway `nullopt`.
     * @param k is a key of an element
     */
    std::optional<SRefObj> get(const Key &k) {
      [[likely]] if (auto it = items_.find(k); it != items_.end()) {
        auto obj = it->second.lock();
        BOOST_ASSERT(obj);
        return obj;
      }
      return std::nullopt;
    }

    /**
     * @brief returns an existed element or creates new one and store it in
     * cache.
     * @param k is a key of an element
     * @param f is a functor to create RefObj
     */
    template <typename F>
    SRefObj get_or_insert(const Key &k, F &&f) {
      [[likely]] if (auto it = items_.find(k); it != items_.end()) {
        auto obj = it->second.lock();
        BOOST_ASSERT(obj);
        return obj;
      }

      auto obj = std::make_shared<RefObj>(std::forward<F>(f)());
      auto [it, inserted] = items_.emplace(k, obj);
      BOOST_ASSERT(inserted);

      BOOST_ASSERT(this->shared_from_this());
      obj->store_internal_refs(it, this->shared_from_this());
      return obj;
    }

   private:
    RefCache() = default;

    void remove(const Iterator &it) {
      items_.erase(it);
    }

    friend struct RefObj;
    Container items_;
  };

}  // namespace kagome