/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_MAP_HPP
#define KAGOME_GENERIC_MAP_HPP

#include "storage/concept/iterable_map.hpp"
#include "storage/concept/readable_map.hpp"
#include "storage/concept/writeable_map.hpp"

namespace kagome::storage::concept {

  /**
   * @brief An abstraction over readable, writeable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericMap : public IterableMap<K, V>,
                      public ReadableMap<K, V>,
                      public WriteableMap<K, V> {
    ~GenericMap() override = default;
  };
}  // namespace kagome::storage::concept

#endif  // KAGOME_GENERIC_MAP_HPP
