/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_HPP
#define KAGOME_READABLE_HPP

#include <outcome/outcome.hpp>

#include "storage/face/owned_or_view.hpp"

namespace kagome::storage::face {

  template <typename K>
  struct ReadableBase {
    using Key = K;

    virtual ~ReadableBase() = default;

    /**
     * @brief Checks if given key-value binding exists in the storage.
     * @param key K
     * @return true if key has value, false if does not, or error at .
     */
    virtual outcome::result<bool> contains(const Key &key) const = 0;

    /**
     * @brief Returns true if the storage is empty.
     */
    virtual bool empty() const = 0;
  };

  /**
   * @brief A mixin for read-only map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct ReadableMap : public ReadableBase<K> {
    using Key = K;

    virtual ~ReadableMap() = default;

    /**
     * @brief Get value by key
     * @param key K
     * @return V
     */
    virtual outcome::result<OwnedOrView<V>> get(const Key &key) const = 0;

    /**
     * @brief Get value by key
     * @param key K
     * @return V if contains(K) or std::nullopt
     */
    virtual outcome::result<std::optional<OwnedOrView<V>>> tryGet(
        const Key &key) const = 0;
  };

  template <typename K, typename V>
  struct ReadableStorage : public ReadableBase<K> {
    using Key = K;

    virtual ~ReadableStorage() = default;

    /**
     * @brief Load value by key
     * @param key K
     * @return V
     */
    virtual outcome::result<OwnedOrView<V>> load(const Key &key) const = 0;

    /**
     * @brief Load value by key
     * @param key K
     * @return V if contains(K) or std::nullopt
     */
    virtual outcome::result<std::optional<OwnedOrView<V>>> tryLoad(
        const Key &key) const = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_READABLE_HPP
