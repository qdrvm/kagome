/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_STORAGE_HPP
#define KAGOME_GENERIC_STORAGE_HPP

#include "storage/face/generic_maps.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericStorage : public ReadOnlyMap<K, V>,
                          public BatchWriteMap<K, V> {};

}  // namespace kagome::face

#endif  // KAGOME_GENERIC_STORAGE_HPP
