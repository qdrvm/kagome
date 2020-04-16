/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_HASHER_HASHER_HPP_
#define KAGOME_CORE_HASHER_HASHER_HPP_

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::crypto {
  class Hasher {
   protected:
    using Hash64 = common::Hash64;
    using Hash128 = common::Hash128;
    using Hash256 = common::Hash256;

   public:
    virtual ~Hasher() = default;

    /**
     * @brief twox_128 calculates 16-byte twox hash
     * @param buffer source buffer
     * @return 128-bit hash value
     */
    virtual Hash64 twox_64(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief twox_128 calculates 16-byte twox hash
     * @param buffer source buffer
     * @return 128-bit hash value
     */
    virtual Hash128 twox_128(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief twox_256 calculates 32-byte twox hash
     * @param buffer source buffer
     * @return 256-bit hash value
     */
    virtual Hash256 twox_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief blake2b_256 function calculates 32-byte blake2b hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2b_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief keccak_256 function calculates 32-byte keccak hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 keccak_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief blake2s_256 function calculates 32-byte blake2s hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 blake2s_256(gsl::span<const uint8_t> buffer) const = 0;

    /**
     * @brief sha2_256 function calculates 32-byte sha2-256 hash
     * @param buffer source value
     * @return 256-bit hash value
     */
    virtual Hash256 sha2_256(gsl::span<const uint8_t> buffer) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_HASHER_HASHER_HPP_
