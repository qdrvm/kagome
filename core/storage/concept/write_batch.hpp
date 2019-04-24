/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITE_BATCH_HPP
#define KAGOME_WRITE_BATCH_HPP

#include "storage/concept/writeable_map.hpp"

namespace kagome::storage::concept {

  /**
   * @brief An abstraction over a storage, which can be used for batch writes
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct WriteBatch : public WriteableMap<K, V> {
    ~WriteBatch() override = default;

    /**
     * @brief Writes batch.
     * @return error code in case of error.
     */
    virtual outcome::result<void> commit() = 0;

    /**
     * @brief Clear batch.
     */
    virtual void clear() = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_WRITE_BATCH_HPP
