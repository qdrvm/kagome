/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {

  /**
   * sr25519 provider error codes
   */
  enum class Sr25519ProviderError {
    SIGN_UNKNOWN_ERROR = 1,  // unknown error occured during call to `sign`
                             // method of bound function
    VERIFY_UNKNOWN_ERROR     // unknown error occured during call to `verify`
                             // method of bound function
  };

  class Sr25519Provider {
   public:
    virtual ~Sr25519Provider() = default;

    /**
     * Generates random keypair for signing the message
     */
    virtual Sr25519Keypair generateKeypair() const = 0;

    /**
     * Generate random keypair from seed
     */
    virtual Sr25519Keypair generateKeypair(const Sr25519Seed &seed) const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     * @param keypair pair of public and secret sr25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<Sr25519Signature> sign(
        const Sr25519Keypair &keypair, gsl::span<const uint8_t> message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const Sr25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Sr25519PublicKey &public_key) const = 0;

    virtual outcome::result<bool> verify_deprecated(
        const Sr25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Sr25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Sr25519ProviderError)

#endif  // KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
