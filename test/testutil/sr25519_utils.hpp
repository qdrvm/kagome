/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  std::array<uint8_t, sr25519_constants::KEYPAIR_SIZE> kp;
  sr25519_keypair_from_seed(kp.data(), seed.data());
  kagome::crypto::SR25519Keypair keypair;
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
