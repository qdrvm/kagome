/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SR25519_UTILS_HPP
#define KAGOME_SR25519_UTILS_HPP

#include "crypto/sr25519_types.hpp"

namespace sr25519_constants = kagome::crypto::constants::sr25519;

/**
 * Generate a SR25519 with some seed
 */
kagome::crypto::Sr25519Keypair generateSr25519Keypair() {
  std::array<uint8_t, sr25519_constants::SEED_SIZE> seed{};
  seed.fill(1);
  std::array<uint8_t, sr25519_constants::KEYPAIR_SIZE> kp;
  sr25519_keypair_from_seed(kp.data(), seed.data());
  kagome::crypto::Sr25519Keypair keypair;
  std::copy(kp.begin(),
            kp.begin() + sr25519_constants::SECRET_SIZE,
            keypair.secret_key.begin());
  std::copy(kp.begin() + sr25519_constants::SECRET_SIZE,
            kp.begin() + sr25519_constants::SECRET_SIZE
                + sr25519_constants::PUBLIC_SIZE,
            keypair.public_key.begin());
  return keypair;
}

#endif  // KAGOME_SR25519_UTILS_HPP
