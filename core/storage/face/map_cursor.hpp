/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_MAP_CURSOR_HPP
#define KAGOME_STORAGE_MAP_CURSOR_HPP

#include <optional>

#include "outcome/outcome.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over generic map cursor.
   * @tparam K key type
   * @tparam V value type
   * @tparam KView key view type
   */
  template <typename K, typename V, typename KView = K>
  struct MapCursor {
    virtual ~MapCursor() = default;

    /**
     * @brief Same as std::begin(...);
     * @return error if any, true if trie is not empty, false otherwise
     */
    virtual outcome::result<bool> seekFirst() = 0;

    /**
     * @brief Find given key and seek iterator to this key.
     * @return error if any, true if \arg key found, false otherwise
     */
    virtual outcome::result<bool> seek(const KView &key) = 0;

    /**
     * @brief Same as std::rbegin(...);, e.g. points to the last valid element
     * @return error if any, true if trie is not empty, false otherwise
     */
    virtual outcome::result<bool> seekLast() = 0;

    /**
     * @brief Is the cursor in a valid state?
     * @return true if the cursor points to an element of the map, false
     * otherwise
     */
    virtual bool isValid() const = 0;

    /**
     * @brief Make step forward.
     */
    virtual outcome::result<void> next() = 0;

    /**
     * @brief Getter for the key of the element currently pointed at.
     * @return key if isValid()
     */
    virtual std::optional<K> key() const = 0;

    /**
     * @brief Getter for value of the element currently pointed at.
     * @return value if isValid()
     */
    virtual std::optional<V> value() const = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_STORAGE_MAP_CURSOR_HPP
