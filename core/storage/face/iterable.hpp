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

#ifndef KAGOME_ITERABLE_HPP
#define KAGOME_ITERABLE_HPP

#include <memory>

#include "storage/face/map_cursor.hpp"

namespace kagome::storage::face {

  /**
   * @brief A mixin for an iterable map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct Iterable {
    virtual ~Iterable() = default;

    /**
     * @brief Returns new key-value iterator.
     * @return kv iterator
     */
    virtual std::unique_ptr<MapCursor<K, V>> cursor() = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_ITERABLE_HPP
