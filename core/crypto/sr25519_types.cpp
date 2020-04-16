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

#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {

  bool VRFOutput::operator==(const VRFOutput &other) const {
    return output == other.output && proof == other.proof;
  }
  bool VRFOutput::operator!=(const VRFOutput &other) const {
    return !(*this == other);
  }

  bool SR25519Keypair::operator==(const SR25519Keypair &other) const {
    return secret_key == other.secret_key && public_key == other.public_key;
  }

  bool SR25519Keypair::operator!=(const SR25519Keypair &other) const {
    return !(*this == other);
  }

}  // namespace kagome::crypto
