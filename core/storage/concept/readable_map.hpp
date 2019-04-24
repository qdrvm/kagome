/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_MAP_HPP
#define KAGOME_READABLE_MAP_HPP

#include <outcome/outcome.hpp>

#include "storage/concept/map_iterator.hpp"

namespace kagome::storage::concept {

  /**
   * @brief An abstraction over read-only MAP.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct ReadableMap {
    virtual ~ReadableMap() = default;

    /**
     * @brief Get value by key
     * @param key K
     * @return V
     */
    virtual outcome::result<V> get(const K &key) const = 0;

    /**
     * @brief Returns true if given key-value binding exists in the storage.
     * @param key K
     * @return true if key has value, false otherwise.
     */
    virtual bool contains(const K &key) const = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_WRITEABLE_KEY_VALUE_HPP
