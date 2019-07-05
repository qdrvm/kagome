/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP

#include <vector>

namespace libp2p::crypto::random {

  /// class RandomGenerator provides interface for random generator
  class RandomGenerator {
   protected:
    using Buffer = std::vector<uint8_t>;

   public:
    virtual ~RandomGenerator() = default;
    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    virtual Buffer randomBytes(size_t len) = 0;
  };

  /**
   * @class CSPRNG provides interface to cryptographic-secure random bytes
   * generator
   */
  class CSPRNG : public RandomGenerator {
   public:
    ~CSPRNG() override = default;

    Buffer randomBytes(size_t len) override = 0;
  };

  /**
   * @class PseudoRandomGenerator provides interface for pseudo-random generator
   */
  class PRNG : public RandomGenerator {
   public:
    ~PRNG() override = default;

    Buffer randomBytes(size_t len) override = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
