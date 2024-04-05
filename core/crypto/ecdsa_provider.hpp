/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bip39/bip39_types.hpp"
#include "crypto/ecdsa_types.hpp"

namespace kagome::crypto {

  class EcdsaProvider {
   public:
    using Keypair = EcdsaKeypair;
    using PublicKey = EcdsaPublicKey;
    using PrivateKey = EcdsaPrivateKey;
    using Seed = EcdsaSeed;

    using Junctions = std::span<const bip39::RawJunction>;

    virtual ~EcdsaProvider() = default;

    virtual outcome::result<EcdsaKeypair> generateKeypair(
        const EcdsaSeed &seed, Junctions junctions) const = 0;

    virtual outcome::result<EcdsaSignature> sign(
        common::BufferView message, const EcdsaPrivateKey &key) const = 0;

    virtual outcome::result<EcdsaSignature> signPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaPrivateKey &key) const = 0;

    virtual outcome::result<bool> verify(common::BufferView message,
                                         const EcdsaSignature &signature,
                                         const EcdsaPublicKey &publicKey,
                                         bool allow_overflow) const = 0;

    virtual outcome::result<bool> verifyPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const = 0;
  };
}  // namespace kagome::crypto
