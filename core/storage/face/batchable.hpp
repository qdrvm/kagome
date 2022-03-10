/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BATCHABLE_HPP
#define KAGOME_BATCHABLE_HPP

#include <memory>

#include "storage/face/write_batch.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {

  /**
   * @brief A mixin for a map that supports batching for efficiency of
   * modifications.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct Batchable {
    virtual ~Batchable() = default;

    /**
     * @brief Creates new Write Batch - an object, which can be used to
     * efficiently write bulk data.
     */
    virtual std::unique_ptr<WriteBatch<K, V>> batch() = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_BATCHABLE_HPP
