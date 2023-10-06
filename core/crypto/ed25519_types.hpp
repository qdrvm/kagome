/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

extern "C" {
#include <schnorrkel/schnorrkel.h>
}

#include "common/blob.hpp"
#include "scale/tie.hpp"

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

}  // namespace kagome::crypto

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519PrivateKey,
                           constants::ed25519::PRIVKEY_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519PublicKey,
                           constants::ed25519::PUBKEY_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519Signature,
                           constants::ed25519::SIGNATURE_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519Seed,
                           constants::ed25519::SEED_SIZE);

namespace kagome::crypto {

  template <typename D>
  struct Ed25519Signed {
    using Type = std::decay_t<D>;
    SCALE_TIE(2);

    Type payload;
    Ed25519Signature signature;
  };

  struct Ed25519Keypair {
    Ed25519PrivateKey secret_key;
    Ed25519PublicKey public_key;

    bool operator==(const Ed25519Keypair &other) const;
    bool operator!=(const Ed25519Keypair &other) const;
  };

  struct Ed25519KeypairAndSeed : Ed25519Keypair {
    Ed25519Seed seed;
  };
}  // namespace kagome::crypto
