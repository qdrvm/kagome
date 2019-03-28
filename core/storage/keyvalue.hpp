/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEYVALUE_HPP
#define KAGOME_KEYVALUE_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace kagome::storage {

  class KeyValue {
   public:
    /**
     * @brief Store value by key
     * @param key non-empty byte buffer
     * @param value byte buffer
     * @return result containing void if put successful, error otherwise
     */
    virtual outcome::result<void> put(const common::Buffer &key,
                                      const common::Buffer &value) = 0;

    /**
     * @brief Get value by key
     * @param key non-empty byte buffer
     * @return value byte buffer
     */
    virtual outcome::result<common::Buffer> get(
        const common::Buffer &key) const = 0;

    /**
     * @brief Delete value by key
     * @param key byte buffer
     */
    virtual void del(const common::Buffer &key) = 0;

    /**
     * @brief Returns true if given key exists in the storage.
     * @param key non-empty buffer
     * @return true if key exists, false otherwise.
     *
     * // TODO(@warchant): in future return type may be replaced with iterators
     */
    virtual bool contains(const common::Buffer &key) const = 0;

    /**
     * @brief Getter for size of this KeyValue map.
     * @return 10 if map stores 10 key-value bindings.
     */
    virtual size_t size() const = 0;

    virtual ~KeyValue() = default;
  };
}  // namespace kagome::storage

#endif  // KAGOME_KEYVALUE_HPP
