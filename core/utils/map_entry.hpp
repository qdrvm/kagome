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
    void insert(M::mapped_type value) {
      assert(not has());
      it_or_key =
          map.emplace(std::move(std::get<K>(it_or_key)), std::move(value))
              .first;
    }
    void insert_or_assign(M::mapped_type value) {
      if (not has()) {
        insert(std::move(value));
      } else {
        **this = std::move(value);
      }
    }
    M::mapped_type remove() {
      assert(has());
      auto node = map.extract(std::get<I>(it_or_key));
      it_or_key = std::move(node.key());
      return std::move(node.mapped());
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    M &map;
    std::variant<I, K> it_or_key{};
  };

  template <typename K, typename V>
  auto entry(std::map<K, V> &map, const K &key) {
    return MapEntry<std::remove_cvref_t<decltype(map)>>{map, key};
  }

  template <typename K, typename V>
  auto entry(std::unordered_map<K, V> &map, const K &key) {
    return MapEntry<std::remove_cvref_t<decltype(map)>>{map, key};
  }
}  // namespace kagome
