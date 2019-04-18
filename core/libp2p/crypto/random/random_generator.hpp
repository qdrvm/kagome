/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP

#include <random>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::crypto::random {

  /// class PseudoRandomGenerator provides interface for random generator
  class PseudoRandomGenerator {
   public:
    /**
     * @brief generators random bytes
     * @param out pointer to buffer
     * @param len number of bytes
     */
    virtual void randomBytes(unsigned char *out, size_t len) = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
