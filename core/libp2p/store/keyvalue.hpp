/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEYVALUE_HPP
#define KAGOME_KEYVALUE_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::store {

  template <typename K, typename V>
  class KeyValue {
   public:
    /**
     * @brief Store value by key
     * @param key
     * @param value
     * @return result containing void if put successful, error otherwise
     */
    virtual outcome::result<void> put(K &&key, V &&value) = 0;

    /**
     * @brief Get value by key
     * @param key
     * @return value
     */
    virtual outcome::result<V> get(K &&key) const = 0;

    /**
     * @brief Delete value by key
     * @param key
     * @return result containing void if del successful, error otherwise
     */
    virtual outcome::result<void> del(K &&key) = 0;

    /**
     * @brief Returns true if given key exists in the storage.
     * @param key
     * @return true if key exists, false otherwise.
     */
    virtual bool contains(K &&key) const = 0;

    /**
     * @brief Getter for size of this KeyValue map.
     * @return 10 if map stores 10 key-value bindings.
     */
    virtual size_t size() const = 0;

    virtual ~KeyValue() = default;
  };

  using StringKV = KeyValue<std::string, std::string>;

}  // namespace libp2p::store

#endif  // KAGOME_KEYVALUE_HPP
