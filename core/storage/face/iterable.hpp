/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ITERABLE_HPP
#define KAGOME_ITERABLE_HPP

#include <memory>

#include "storage/face/map_cursor.hpp"

namespace kagome::storage::face {

  /**
   * @brief A mixin for an iterable map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct Iterable {
    virtual ~Iterable() = default;

    /**
     * @brief Returns new key-value iterator.
     * @return kv iterator
     */
    virtual std::unique_ptr<MapCursor<K, V>> cursor() = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_ITERABLE_HPP
