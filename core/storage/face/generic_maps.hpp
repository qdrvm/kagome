/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_MAPS_HPP
#define KAGOME_GENERIC_MAPS_HPP

#include "storage/face/batch_writeable.hpp"
#include "storage/face/iterable.hpp"
#include "storage/face/readable.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {
  /**
   * @brief An abstraction over a readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericStorage : Readable<K, V>,
                          Iterable<K, V>,
                          Writeable<K, V>,
                          BatchWriteable<K, V> {
    /**
     * Reports RAM state size
     * @return size in bytes
     */
    virtual size_t size() const {
      abort();
    }
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_GENERIC_MAPS_HPP
