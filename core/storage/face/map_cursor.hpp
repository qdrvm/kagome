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

#ifndef KAGOME_MAP_CURSOR_HPP
#define KAGOME_MAP_CURSOR_HPP

namespace kagome::storage::face {

  /**
   * @brief An abstraction over generic map cursor.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct MapCursor {
    virtual ~MapCursor() = default;

    /**
     * @brief Same as std::begin(...);
     */
    virtual void seekToFirst() = 0;

    /**
     * @brief Find given key and seek iterator to this key.
     */
    virtual void seek(const K &key) = 0;

    /**
     * @brief Same as std::rbegin(...);, e.g. points to the last valid element
     */
    virtual void seekToLast() = 0;

    /**
     * @brief Is iterator valid?
     * @return true if iterator points to the element of map, false otherwise
     */
    virtual bool isValid() const = 0;

    /**
     * @brief Make step forward.
     */
    virtual void next() = 0;

    /**
     * @brief Make step backwards.
     */
    virtual void prev() = 0;

    /**
     * @brief Getter for key.
     * @return key
     */
    virtual K key() const = 0;

    /**
     * @brief Getter for value.
     * @return value
     */
    virtual V value() const = 0;
  };

}  // namespace kagome::storage::face

#endif  //KAGOME_MAP_CURSOR_HPP
