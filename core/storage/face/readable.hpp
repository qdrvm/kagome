/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_HPP
#define KAGOME_READABLE_HPP

#include <outcome/outcome.hpp>
#include "storage/face/map_cursor.hpp"

namespace kagome::storage::face {

  /**
   * @brief A mixin for read-only map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct Readable {
    virtual ~Readable() = default;

    /**
     * @brief Get value by key
     * @param key K
     * @return V
     */
    virtual outcome::result<V> get(const K &key) const = 0;

    /**
     * @brief Get value by key
     * @param key K
     * @return V if contains(K) or boost::none
     */
     virtual outcome::result<boost::optional<V>> try_get(const K &key) const {
       if (contains(key)) {
         auto res = get(key);
         if (res.has_value()) return res.value();
         return res.as_failure();
       }
       return boost::none;
     }

    /**
     * @brief Returns true if given key-value binding exists in the storage.
     * @param key K
     * @return true if key has value, false otherwise.
     */
    virtual bool contains(const K &key) const = 0;

    /**
     * @brief Returns true if the storage is empty.
     */
    virtual bool empty() const = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_WRITEABLE_KEY_VALUE_HPP
