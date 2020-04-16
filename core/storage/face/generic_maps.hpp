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

#ifndef KAGOME_GENERIC_MAPS_HPP
#define KAGOME_GENERIC_MAPS_HPP

#include "storage/face/batchable.hpp"
#include "storage/face/iterable.hpp"
#include "storage/face/readable.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over a readable and iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct ReadOnlyMap : public Iterable<K, V>, public Readable<K, V> {};

  /**
   * @brief An abstraction over a readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericMap : public ReadOnlyMap<K, V>, public Writeable<K, V> {};

  /**
   * @brief An abstraction over a writeable key-value map with batching support.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct BatchWriteMap : public Writeable<K, V>, public Batchable<K, V> {};
}  // namespace kagome::storage::face

#endif  // KAGOME_GENERIC_MAPS_HPP
