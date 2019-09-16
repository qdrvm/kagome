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

  enum class ED25519ProviderError {
    FAILED_GENERATE_KEYPAIR = 1,
    SIGN_UNKNOWN_ERROR,   // unknown error occurred during call to `sign` method
                          // of bound function
    VERIFY_UNKNOWN_ERROR  // unknown error occured during call to `verify`
                          // method of bound function
  };

  class ED25519Provider {
   public:
    virtual ~ED25519Provider() = default;

    /**
     * Generates random keypair for signing the message
     * @return ed25519 key pair if succeeded of error if failed
     */
    virtual outcome::result<ED25519Keypair> generateKeypair() const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     * @param keypair pair of public and private ed25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<ED25519Signature> sign(
        const ED25519Keypair &keypair, gsl::span<uint8_t> message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const ED25519Signature &signature,
        gsl::span<uint8_t> message,
        const ED25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, ED25519ProviderError)

#endif  // KAGOME_CORE_CRYPTO_ED25519_PROVIDER_HPP
