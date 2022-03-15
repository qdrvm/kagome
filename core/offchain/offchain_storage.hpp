/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINSTORAGE
#define KAGOME_OFFCHAIN_OFFCHAINSTORAGE

#include <boost/optional.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::offchain {

  /**
   * A wrapper for a storage of offchain data
   * Provides a convenient interface to work with it
   */
  class OffchainStorage {
   public:
    virtual ~OffchainStorage() = default;

    /**
     * @brief Sets a value in the storage
     * @param key a pointer-size indicating the key
     * @param value a pointer-size indicating the value
     * @return success or error
     */
    virtual outcome::result<void> set(const common::Buffer &key,
                                      common::Buffer value) = 0;

    /**
     * @brief Remove a value from the local storage
     * @param key a pointer-size indicating the key
     * @return success or error
     */
    virtual outcome::result<void> clear(const common::Buffer &key) = 0;

    /**
     * @brief Sets a new value in the local storage if the condition matches the
     * current value
     * @param key a pointer-size indicating the key
     * @param expected a pointer-size indicating the old value
     * @param value a pointer-size indicating the new value
     * @return bool as result, or error at failure
     */
    virtual outcome::result<bool> compare_and_set(
        const common::Buffer &key,
        std::optional<std::reference_wrapper<const common::Buffer>> expected,
        common::Buffer value) = 0;

    /**
     * @brief Gets a value from the local storage
     * @param key a pointer-size indicating the key
     * @return value for success, or error at failure
     */
    virtual outcome::result<common::Buffer> get(const common::Buffer &key) = 0;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINSTORAGE
