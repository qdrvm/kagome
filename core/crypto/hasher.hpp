/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_HASHER_HASHER_HPP_
#define KAGOME_CORE_HASHER_HASHER_HPP_

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::hash {
  class Hasher {
   protected:
    using Hash128 = common::Hash128;
    using Hash256 = common::Hash256;

   public:
    virtual ~Hasher() = default;

    /**
     * @brief hashTwox128 calculates 16-byte twox hash
     * @param buffer source buffer
     * @return 128-bit hash value
     */
    virtual Hash128 twox_128(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief hashTwox256 calculates 32-byte twox hash
     * @param buffer source buffer
     * @return 256-bit hash value
     */
    virtual Hash256 twox_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief hashBlake2b_256 function calculates 32-byte blake2b hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2b_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief hashBlake2s_256 function calculates 32-byte blake2s hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2s_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief hashSha2_256 function calculates 32-byte sha-2-256 hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 sha2_256(gsl::span<const uint8_t> buffer) const = 0;
  };
}  // namespace kagome::hash

#endif  // KAGOME_CORE_HASHER_HASHER_HPP_
