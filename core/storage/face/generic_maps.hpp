/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/face/batch_writeable.hpp"
#include "storage/face/iterable.hpp"
#include "storage/face/readable.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over a readable, writeable, iterable, batchable
   * key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericStorage : Readable<K, V>, Iterable<K, V>, Writeable<K, V> {
    /**
     * Reports RAM state size
     * @return size in bytes
     */
    virtual std::optional<size_t> byteSizeHint() const {
      return std::nullopt;
    }
  };

  template <typename K, typename V>
  struct BatchableStorage : GenericStorage<K, V>, BatchWriteable<K, V> {};

}  // namespace kagome::storage::face
