/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/non_copyable.hpp"

#include <unordered_map>

namespace kagome::parachain {

  template <typename K, typename V>
  using StorePair = std::pair<std::decay_t<K>, std::decay_t<V>>;

  template <typename T>
  struct StoreUnit {
    static constexpr size_t kDebugHardLimit = 10'000ull;

    using K = typename T::first_type;
    using V = typename T::second_type;

    StoreUnit() = default;
    ~StoreUnit() = default;

    StoreUnit(StoreUnit &&) = default;
    StoreUnit(const StoreUnit &) = delete;

    StoreUnit &operator=(StoreUnit &&) = default;
    StoreUnit &operator=(const StoreUnit &) = delete;

    std::optional<std::reference_wrapper<V>> get(const K &k) {
      assert(store_.size() < kDebugHardLimit);
      if (auto it = store_.find(k); it != store_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    template <typename... A>
    std::reference_wrapper<V> get_or_create(const K &k, A &&...args) {
      assert(store_.size() < kDebugHardLimit);
      if (auto it = store_.find(k); it != store_.end()) {
        return it->second;
      }
      return set(k, V{std::forward<A>(args)...});
    }

    std::optional<std::reference_wrapper<const V>> get(const K &k) const {
      assert(store_.size() < kDebugHardLimit);
      if (auto it = store_.find(k); it != store_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<V> extract(const K &k) {
      assert(store_.size() < kDebugHardLimit);
      if (auto it = store_.find(k); it != store_.end()) {
        V v{std::move(it->second)};
        store_.erase(it);
        return std::move(v);
      }
      return std::nullopt;
    }

    std::reference_wrapper<V> set(const K &k, V &&v) {
      assert(store_.size() < kDebugHardLimit);
      return store_.template insert_or_assign(k, std::move(v)).first->second;
    }

   private:
    std::unordered_map<K, V> store_;
  };

  template <typename T, typename... A>
  struct Store : StoreUnit<T>, Store<A...> {};

  template <typename T>
  struct Store<T> : StoreUnit<T> {};

  template <typename T, typename... A>
  inline constexpr StoreUnit<T> &as(Store<A...> &ref) {
    return static_cast<StoreUnit<T> &>(ref);
  }

  template <typename T, typename... A>
  inline constexpr const StoreUnit<T> &as(const Store<A...> &ref) {
    return static_cast<const StoreUnit<T> &>(ref);
  }
}  // namespace kagome::parachain
