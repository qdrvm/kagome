/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_BOOST_GENERATOR_HPP
#define KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_BOOST_GENERATOR_HPP

#include "libp2p/crypto/random_generator/boost_generator.hpp"

namespace kagome::crypto {

  using BoostRandomGenerator = libp2p::crypto::random::BoostRandomGenerator;
  using CSPRNG = libp2p::crypto::random::CSPRNG;

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_RANDOM_GENERATOR_BOOST_GENERATOR_HPP
