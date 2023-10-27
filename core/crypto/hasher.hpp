/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer_view.hpp"

namespace kagome::crypto {
  class Hasher {
   protected:
    using Hash64 = common::Hash64;
    using Hash128 = common::Hash128;
    using Hash256 = common::Hash256;
    using Hash512 = common::Hash512;

   public:
    virtual ~Hasher() = default;

    /**
     * @brief twox_64 calculates 8-byte twox hash
     * @param data source data
     * @return 64-bit hash value
     */
    virtual Hash64 twox_64(common::BufferView data) const = 0;

    /**
     * @brief blake2b_64 function calculates 8-byte blake2b hash
     * @param data source value
     * @return 64-bit hash value
     */
    virtual Hash64 blake2b_64(common::BufferView data) const = 0;

    /**
     * @brief twox_128 calculates 16-byte twox hash
     * @param data source data
     * @return 128-bit hash value
     */
    virtual Hash128 twox_128(common::BufferView data) const = 0;

    /**
     * @brief blake2b_128 function calculates 16-byte blake2b hash
     * @param data source value
     * @return 128-bit hash value
     */
    virtual Hash128 blake2b_128(common::BufferView data) const = 0;

    /**
     * @brief twox_256 calculates 32-byte twox hash
     * @param data source data
     * @return 256-bit hash value
     */
    virtual Hash256 twox_256(common::BufferView data) const = 0;

    /**
     * @brief blake2b_256 function calculates 32-byte blake2b hash
     * @param data source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2b_256(common::BufferView data) const = 0;

    /**
     * @brief blake2b_512 function calculates 64-byte blake2b hash
     * @param data source value
     * @return 512-bit hash value
     */
    virtual Hash512 blake2b_512(common::BufferView data) const = 0;

    /**
     * @brief keccak_256 function calculates 32-byte keccak hash
     * @param data source value
     * @return 256-bit hash value
     */
    virtual Hash256 keccak_256(common::BufferView data) const = 0;

    /**
     * @brief blake2s_256 function calculates 32-byte blake2s hash
     * @param data source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2s_256(common::BufferView data) const = 0;

    /**
     * @brief sha2_256 function calculates 32-byte sha2-256 hash
     * @param data source value
     * @return 256-bit hash value
     */
    virtual Hash256 sha2_256(common::BufferView data) const = 0;
  };
}  // namespace kagome::crypto
