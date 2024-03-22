/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/sr25519_types.hpp"

namespace sr25519_constants = kagome::crypto::constants::sr25519;

/**
 * Generate a SR25519 with some seed
 */
kagome::crypto::Sr25519Keypair generateSr25519Keypair() {
  namespace crypto = kagome::crypto;
  std::array<uint8_t, sr25519_constants::SEED_SIZE> seed{};
  seed.fill(1);
  std::array<uint8_t, sr25519_constants::KEYPAIR_SIZE> kp{};
  sr25519_keypair_from_seed(kp.data(), seed.data());
  crypto::Sr25519Keypair keypair{
      crypto::Sr25519SecretKey::from(crypto::SecureCleanGuard(
          std::span(kp).subspan<0, SR25519_SECRET_SIZE>())),
      crypto::Sr25519PublicKey::fromSpan(
          std::span{kp}.subspan(SR25519_SECRET_SIZE, SR25519_PUBLIC_SIZE))
          .value()};
  return keypair;
}
