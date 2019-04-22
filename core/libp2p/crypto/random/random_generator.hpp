/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto::random {

  /// class RandomGenerator provides interface for random generator
  class RandomGenerator {
   protected:
    using Buffer = kagome::common::Buffer;
   public:
    virtual ~RandomGenerator() = default;
    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    virtual Buffer randomBytes(size_t len) = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
