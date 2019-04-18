/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP

#include "libp2p/crypto/random/random_generator.hpp"

namespace libp2p::crypto::random {
  /**
   * @class CryptographicSafeRandomProvider provides interface
   * to cryptographic-secure random bytes generator
   */
  class CryptographicallySecureRandomGenerator : public PseudoRandomGenerator {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief generators random bytes
     * @param out pointer to buffer
     * @param len number of bytes
     */
    void randomBytes(unsigned char *out, size_t len) override = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_CRYPTOGRAPHIC_SAFE_RANDOM_PROVIDER_HPP
