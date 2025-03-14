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
#include "crypto/common.hpp"

namespace kagome::crypto::constants::ed25519 {
  /**
   * Important constants to deal with ed25519
   */
  enum {  // NOLINT(performance-enum-size)
    PRIVKEY_SIZE = ED25519_SECRET_KEY_LENGTH,
    PUBKEY_SIZE = ED25519_PUBLIC_KEY_LENGTH,
    SIGNATURE_SIZE = ED25519_SIGNATURE_LENGTH,
    SEED_SIZE = PRIVKEY_SIZE,
  };
}  // namespace kagome::crypto::constants::ed25519

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519PublicKey,
                           constants::ed25519::PUBKEY_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Ed25519Signature,
                           constants::ed25519::SIGNATURE_SIZE);

namespace kagome::crypto {

  struct Ed25519KeyTag;
  using Ed25519PrivateKey =
      PrivateKey<constants::ed25519::PRIVKEY_SIZE, Ed25519KeyTag>;

  struct Ed25519SeedTag;
  using Ed25519Seed = PrivateKey<constants::ed25519::SEED_SIZE, Ed25519SeedTag>;

  template <typename D>
  struct Ed25519Signed {
    using Type = std::decay_t<D>;
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
