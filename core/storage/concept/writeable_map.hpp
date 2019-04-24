/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITEABLE_MAP_HPP
#define KAGOME_WRITEABLE_MAP_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage::concept {

  /**
   * @brief An abstraction over modifiable map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct WriteableMap {
    virtual ~WriteableMap() = default;

    /**
     * @brief Store value by key
     * @param key key
     * @param value value
     * @return result containing void if put successful, error otherwise
     */
    virtual outcome::result<void> put(const K &key, const V &value) = 0;

    /**
     * @brief Remove value by key
     * @param key K
     * @return error code if error happened
     */
    virtual outcome::result<void> remove(const K &key) = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_WRITEABLE_MAP_HPP
