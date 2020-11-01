/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
#define KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP

extern "C" {
#include <schnorrkel/schnorrkel.h>
}

#include "common/blob.hpp"

namespace kagome::crypto {

  namespace constants::ed25519 {
    /**
     * Important constants to deal with ed25519
     */
    enum {
      PRIVKEY_SIZE = ED25519_SECRET_KEY_LENGTH,
      PUBKEY_SIZE = ED25519_PUBLIC_KEY_LENGTH,
      SIGNATURE_SIZE = ED25519_SIGNATURE_LENGTH,
      SEED_SIZE = PRIVKEY_SIZE,
    };
  }  // namespace constants::ed25519

  using Ed25519PrivateKey = common::Blob<constants::ed25519::PRIVKEY_SIZE>;
  using Ed25519PublicKey = common::Blob<constants::ed25519::PUBKEY_SIZE>;

  struct Ed25519Keypair {
    Ed25519PrivateKey secret_key;
    Ed25519PublicKey public_key;

    bool operator==(const Ed25519Keypair &other) const;
    bool operator!=(const Ed25519Keypair &other) const;
  };

  using Ed25519Signature = common::Blob<constants::ed25519::SIGNATURE_SIZE>;

  using Ed25519Seed = common::Blob<constants::ed25519::SEED_SIZE>;
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
