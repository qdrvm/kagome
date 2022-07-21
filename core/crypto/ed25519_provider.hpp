/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED25519_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_ED25519_PROVIDER_HPP

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "crypto/ed25519_types.hpp"

namespace kagome::crypto {

  class Ed25519Provider {
   public:
    virtual ~Ed25519Provider() = default;

    /**
     * Generates random keypair for signing the message
     * @return ed25519 key pair if succeeded of error if failed
     */
    virtual Ed25519KeypairAndSeed generateKeypair() const = 0;

    /**
     * @brief generates key pair by seed
     * @param seed seed value
     * @return ed25519 key pair
     */
    virtual Ed25519Keypair generateKeypair(const Ed25519Seed &seed) const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     * @param keypair pair of public and private ed25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<Ed25519Signature> sign(
        const Ed25519Keypair &keypair, gsl::span<uint8_t> message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const Ed25519Signature &signature,
        gsl::span<uint8_t> message,
        const Ed25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_PROVIDER_HPP
