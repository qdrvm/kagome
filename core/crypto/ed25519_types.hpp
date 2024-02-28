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

// KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
//                            Ed25519PrivateKey,
//                            constants::ed25519::PRIVKEY_SIZE);
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

  class Ed25519PrivateKey
      : public common::SLBuffer<constants::ed25519::PRIVKEY_SIZE,
                                SecureHeapAllocator<uint8_t>> {
   public:
    Ed25519PrivateKey(const Ed25519PrivateKey &) = delete;
    Ed25519PrivateKey &operator=(const Ed25519PrivateKey &) = delete;

    Ed25519PrivateKey(Ed25519PrivateKey &&key) noexcept
        : SLBuffer{std::move(key)} {}

    Ed25519PrivateKey &operator=(Ed25519PrivateKey &&key) noexcept {
      (*this).SLBuffer::operator=(std::move(key));
      return *this;
    }

    // && to highlight that view will be cleared
    static Ed25519PrivateKey from(
        std::span<uint8_t, constants::ed25519::PRIVKEY_SIZE> &&view) {
      Ed25519PrivateKey key;
      key.put(view);
      OPENSSL_cleanse(view.data(), view.size());
      return key;
    }
    static outcome::result<Ed25519PrivateKey> from(std::span<uint8_t> &&view) {
      if (view.size() != constants::ed25519::PRIVKEY_SIZE) {
        return common::BlobError::INCORRECT_LENGTH;
      }
      return Ed25519PrivateKey::from(
          std::span<uint8_t, constants::ed25519::PRIVKEY_SIZE>{view});
    }

   private:
    Ed25519PrivateKey() {}
  };

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
