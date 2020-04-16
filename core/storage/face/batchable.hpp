/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
