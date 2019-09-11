/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP

#include <gsl/span.h>
#include <outcome/outcome.hpp>
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {
  class SR25519Provider {
   public:
    virtual ~SR25519Provider() = default;

    /**
     * Generates random keypair for signing the message
     */
    virtual SR25519Keypair generateKeypair() const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     */
    virtual outcome::result<SR25519Signature> sign(
        gsl::span<uint8_t> signature,
        const SR25519Keypair &keypair,
        gsl::span<uint8_t> message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual bool verify(gsl::span<uint8_t> signature,
                        gsl::span<uint8_t> message,
                        const SR25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
