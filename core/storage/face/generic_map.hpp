/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_MAP_HPP
#define KAGOME_GENERIC_MAP_HPP

#include "storage/face/iterable_map.hpp"
#include "storage/face/readable_map.hpp"
#include "storage/face/writeable_map.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericMap : virtual public IterableMap<K, V>,
                      virtual public ReadableMap<K, V>,
                      virtual public WriteableMap<K, V> {};
}  // namespace kagome::storage::face

#endif  // KAGOME_GENERIC_MAP_HPP
