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

#ifndef KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
#define KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP

#include <ed25519/ed25519.h>
#include "common/blob.hpp"

namespace kagome::crypto {

  namespace constants::ed25519 {
    /**
     * Important constants to deal with ed25519
     */
    enum {
      PRIVKEY_SIZE = ed25519_privkey_SIZE,
      PUBKEY_SIZE = ed25519_pubkey_SIZE,
      SIGNATURE_SIZE = ed25519_signature_SIZE
    };
  }  // namespace constants::ed25519

  using ED25519PrivateKey = common::Blob<constants::ed25519::PRIVKEY_SIZE>;
  using ED25519PublicKey = common::Blob<constants::ed25519::PUBKEY_SIZE>;

  struct ED25519Keypair {
    ED25519PrivateKey private_key;
    ED25519PublicKey public_key;

    bool operator==(const ED25519Keypair &other) const;
    bool operator!=(const ED25519Keypair &other) const;
  };

  using ED25519Signature = common::Blob<constants::ed25519::SIGNATURE_SIZE>;
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
