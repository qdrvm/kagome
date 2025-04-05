/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"

namespace sr25519_constants = kagome::crypto::constants::sr25519;

/**
 * Generate a SR25519 with some seed
 */
kagome::crypto::Sr25519Keypair generateSr25519Keypair(
    kagome::consensus::AuthorityIndex auth_id_as_seed) {
  namespace crypto = kagome::crypto;
  std::array<uint8_t, sr25519_constants::SEED_SIZE> seed{};
  for (size_t i = 0; i < seed.size(); ++i) {
    seed[i] = reinterpret_cast<const uint8_t *>(  // NOLINT
        &auth_id_as_seed)[i % sizeof(auth_id_as_seed)];
  }
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
