/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PERSISTED_MAP_HPP
#define KAGOME_PERSISTED_MAP_HPP

#include "storage/face/generic_map.hpp"
#include "storage/face/write_batch.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over a map accessible via filesystem or remove
   * connection. It supports batching for efficiency of modifications.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct PersistedMap : public GenericMap<K, V> {
    /**
     * @brief Creates new Write Batch - an object, which can be used to
     * efficiently write bulk data.
     */
    virtual std::unique_ptr<WriteBatch<K, V>> batch() = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_PERSISTED_MAP_HPP
