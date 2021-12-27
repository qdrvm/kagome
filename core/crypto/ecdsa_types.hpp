/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ECDSA_TYPES_HPP
#define KAGOME_CORE_CRYPTO_ECDSA_TYPES_HPP

#include "libp2p/crypto/ecdsa_types.hpp"

#include "common/blob.hpp"

namespace kagome::crypto {
  using namespace libp2p::crypto::ecdsa;

  namespace constants::ecdsa {
    enum {
      PRIVKEY_SIZE = sizeof(PrivateKey),
      PUBKEY_SIZE = sizeof(PublicKey),
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

namespace kagome::crypto {

  struct EcdsaKeypair {
    EcdsaPrivateKey secret_key;
    EcdsaPublicKey public_key;

    bool operator==(const EcdsaKeypair &other) const;
    bool operator!=(const EcdsaKeypair &other) const;
  };

  using EcdsaSignature = std::vector<uint8_t>;
  using EcdsaSeed = common::Blob<constants::ecdsa::SEED_SIZE>;

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ECDSA_TYPES_HPP
