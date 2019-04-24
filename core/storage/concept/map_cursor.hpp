/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MAP_CURSOR_HPP
#define KAGOME_MAP_CURSOR_HPP

namespace kagome::storage::concept {

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

}  // namespace kagome::storage::concept

#endif  //KAGOME_MAP_CURSOR_HPP
