/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_GENERATOR_HPP

#include <random>

#include "libp2p/crypto/random/prng.hpp"

namespace libp2p::crypto::random {
  /**
   * @class StdRandomGenerator provides implementation of pseudo-random bytes
   * generator it can be cryptographic-secure on platforms which provide
   * qualitative source of entropy on platforms which don't, it falls back to
   * pseudo-random algorithm
   */
  class StdRandomGenerator : public PRNG {
   public:
    ~StdRandomGenerator() override = default;
    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    Buffer randomBytes(size_t len) override;

   private:
    /// std random generator
    std::random_device generator_;
    /// uniform distribution tool
    std::uniform_int_distribution<uint8_t> distribution_;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_GENERATOR_HPP
