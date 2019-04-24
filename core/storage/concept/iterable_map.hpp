/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ITERABLE_MAP_HPP
#define KAGOME_ITERABLE_MAP_HPP

#include <memory>

#include "storage/concept/map_cursor.hpp"

namespace kagome::storage::concept {

  /**
   * @brief An abstraction of a key-value map, that is iterable.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct IterableMap {
    virtual ~IterableMap() = default;

    /**
     * @brief Returns new key-value iterator.
     * @return kv iterator
     */
    virtual std::unique_ptr<MapCursor<K, V>> cursor() = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_ITERABLE_MAP_HPP
