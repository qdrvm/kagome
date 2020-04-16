/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
