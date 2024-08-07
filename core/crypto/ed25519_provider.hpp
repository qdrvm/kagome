/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bip39/bip39_types.hpp"
#include "crypto/ed25519_types.hpp"

namespace kagome::crypto {

  class Ed25519Provider {
   public:
    using Keypair = Ed25519Keypair;
    using PublicKey = Ed25519PublicKey;
    using PrivateKey = Ed25519PrivateKey;
    using Seed = Ed25519Seed;

    using Junctions = std::span<const bip39::RawJunction>;

    virtual ~Ed25519Provider() = default;

    /**
     * @brief generates key pair by seed
     * @param seed seed value
     * @return ed25519 key pair
     */
    virtual outcome::result<Ed25519Keypair> generateKeypair(
        const Ed25519Seed &seed, Junctions junctions) const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     * @param keypair pair of public and private ed25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<Ed25519Signature> sign(
        const Ed25519Keypair &keypair, common::BufferView message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const Ed25519Signature &signature,
        common::BufferView message,
        const Ed25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto
