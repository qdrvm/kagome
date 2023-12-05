/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "storage/face/map_cursor.hpp"

namespace kagome::storage::face {

  /**
   * @brief A mixin for an iterable map.
   * @tparam K map key type
   * @tparam V map value type
   */
  template <typename K, typename V>
  struct Iterable {
    using Cursor = MapCursor<K, V>;

    virtual ~Iterable() = default;

    /**
     * @brief Returns new key-value iterator.
     * @return kv iterator
     */
    virtual std::unique_ptr<Cursor> cursor() = 0;
  };

}  // namespace kagome::storage::face
