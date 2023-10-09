/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/secp256k1_types.hpp"

namespace kagome::crypto {
  namespace constants::ecdsa {
    enum {
      PRIVKEY_SIZE = 32,
      PUBKEY_SIZE = secp256k1::constants::kCompressedPublicKeySize,
      SIGNATURE_SIZE = secp256k1::constants::kCompactSignatureSize,
      SEED_SIZE = PRIVKEY_SIZE,
    };
  }
}  // namespace kagome::crypto

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           EcdsaPrivateKey,
                           constants::ecdsa::PRIVKEY_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           EcdsaPublicKey,
                           constants::ecdsa::PUBKEY_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           EcdsaSignature,
                           constants::ecdsa::SIGNATURE_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           EcdsaSeed,
                           constants::ecdsa::SEED_SIZE);

namespace kagome::crypto {

  struct EcdsaKeypair {
    EcdsaPrivateKey secret_key;
    EcdsaPublicKey public_key;

    bool operator==(const EcdsaKeypair &other) const;
    bool operator!=(const EcdsaKeypair &other) const;
  };

  using EcdsaPrehashedMessage = secp256k1::MessageHash;

  struct EcdsaKeypairAndSeed : EcdsaKeypair {
    EcdsaSeed seed;
  };
}  // namespace kagome::crypto
