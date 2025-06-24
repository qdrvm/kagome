/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cassert>
#include <map>
#include <unordered_map>
#include <variant>

namespace kagome {
  template <typename M>
  struct MapEntry {
    using I = typename M::iterator;
    using K = typename M::key_type;
    using V = typename M::mapped_type;

    MapEntry(M &map, const K &key) : map{map} {
      if (auto it = map.find(key); it != map.end()) {
        it_or_key = it;
      } else {
        it_or_key = key;
      }
    }

    bool has() const {
      return std::holds_alternative<I>(it_or_key);
    }
    operator bool() const {
      return has();
    }
    auto &operator*() {
      assert(has());
      return std::get<I>(it_or_key)->second;
    }
    auto *operator->() {
      assert(has());
      return &std::get<I>(it_or_key)->second;
    }
    void insert(V value) {
      assert(not has());
      it_or_key =
          map.emplace(std::move(std::get<K>(it_or_key)), std::move(value))
              .first;
    }
    void insert_or_assign(V value) {
      if (not has()) {
        insert(std::move(value));
      } else {
        **this = std::move(value);
      }
    }
    V remove() {
      assert(has());
      auto node = map.extract(std::get<I>(it_or_key));
      it_or_key = std::move(node.key());
      return std::move(node.mapped());
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    M &map;
    std::variant<I, K> it_or_key{};
  };

  template <typename M>
  struct MapEntry<const M> {
    MapEntry(const M &map, const typename M::key_type &key)
        : map{map}, it{map.find(key)} {}

    bool has() const {
      return it != map.end();
    }
    operator bool() const {
      return has();
    }
    auto &operator*() {
      assert(has());
      return it->second;
    }
    auto *operator->() {
      assert(has());
      return &it->second;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const M &map;
    typename M::const_iterator it;
  };

  template <typename M>
  MapEntry(M &map, const typename M::key_type &key) -> MapEntry<M>;

  template <typename K, typename V>
  auto entry(std::map<K, V> &map, const K &key) {
    return MapEntry{map, key};
  }

  template <typename K, typename V>
  auto entry(std::unordered_map<K, V> &map, const K &key) {
    return MapEntry{map, key};
  }

  template <typename K, typename V>
  auto entry(const std::map<K, V> &map, const K &key) {
    return MapEntry{map, key};
  }

  template <typename K, typename V>
  auto entry(const std::unordered_map<K, V> &map, const K &key) {
    return MapEntry{map, key};
  }
}  // namespace kagome
