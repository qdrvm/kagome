/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP

#include "libp2p/crypto/random/random_generator.hpp"

namespace libp2p::crypto::random {
  /**
   * @class CSPRNG provides interface to cryptographic-secure random bytes
   * generator
   */
  class CSPRNG : public RandomGenerator {
   public:
    ~CSPRNG() override = default;
    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    Buffer randomBytes(size_t len) override = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP
