/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SR25519_UTILS_HPP
#define KAGOME_SR25519_UTILS_HPP

#include "crypto/sr25519_types.hpp"

namespace sr25519_constants = kagome::crypto::constants::sr25519;

/**
 * Generate a SR25519 with some seed
 */
kagome::crypto::SR25519Keypair generateSR25519Keypair() {
  std::array<uint8_t, sr25519_constants::SEED_SIZE> seed{};
  seed.fill(1);
  std::vector<uint8_t> kp(sr25519_constants::KEYPAIR_SIZE, 0);
  sr25519_keypair_from_seed(kp.data(), seed.data());
  return kagome::crypto::SR25519Keypair{kp};
}

#endif  // KAGOME_SR25519_UTILS_HPP
