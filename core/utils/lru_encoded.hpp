/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

#include <boost/container_hash/hash.hpp>

#include "common/buffer_view.hpp"
#include "utils/lru.hpp"

namespace kagome {
  /**
   * LRU with encoded value deduplication.
   * Used to cache runtime call results.
   */
  template <typename K, typename V>
  class LruEncoded {
   public:
    LruEncoded(size_t capacity) : values_{capacity}, hashes_{capacity} {}

    std::optional<std::shared_ptr<V>> get(const K &k) {
      return values_.get(k);
    }

    std::shared_ptr<V> put(const K &k, const V &v) {
      return put(k, V{v});
    }

    std::shared_ptr<V> put(const K &k, V &&v) {
      return put(k, std::move(v), scale::encode(v).value());
    }

    std::shared_ptr<V> put(const K &k, V &&v, common::BufferView encoded) {
      auto h = hash(encoded);
      auto weak = hashes_.get(h);
      std::shared_ptr<V> shared;
      if (weak) {
        shared = weak->get().lock();
        // check collisions (size_t is weak hash)
        if (shared and *shared != v) {
          shared.reset();
        }
      }
      if (not shared) {
        shared = std::make_shared<V>(std::move(v));
        if (weak) {
          weak->get() = shared;
        } else {
          hashes_.put(h, shared);
        }
      }
      values_.put(k, std::shared_ptr<V>{shared});
      return shared;
    }

    void erase(const K &k) {
      values_.erase(k);
    }

    template <typename F>
    void erase_if(const F &f) {
      values_.erase_if(f);
    }

   private:
    using Hash = size_t;
    Hash hash(common::BufferView v) {
      return boost::hash_range(v.begin(), v.end());
    }

    Lru<K, std::shared_ptr<V>> values_;
    Lru<Hash, std::weak_ptr<V>> hashes_;
  };
}  // namespace kagome
