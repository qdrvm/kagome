/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_MAP_HPP
#define KAGOME_GENERIC_MAP_HPP

#include "storage/face/iterable.hpp"
#include "storage/face/readable.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericMap : public Iterable<K, V>,
                      public Readable<K, V>,
                      public Writeable<K, V> {};
}  // namespace kagome::storage::face

#endif  // KAGOME_GENERIC_MAP_HPP
