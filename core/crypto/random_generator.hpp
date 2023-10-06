/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_HPP
#define KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_HPP

#include "libp2p/crypto/random_generator.hpp"

namespace kagome::crypto {
  using RandomGenerator = libp2p::crypto::random::RandomGenerator;
  using CSPRNG = libp2p::crypto::random::CSPRNG;
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_HPP
