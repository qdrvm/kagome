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

#ifndef KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {

  /**
   * sr25519 provider error codes
   */
  enum class SR25519ProviderError {
    SIGN_UNKNOWN_ERROR = 1,  // unknown error occured during call to `sign`
                             // method of bound function
    VERIFY_UNKNOWN_ERROR  // unknown error occured during call to `verify`
                          // method of bound function
  };

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
     * @param keypair pair of public and secret sr25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<SR25519Signature> sign(
        const SR25519Keypair &keypair, gsl::span<uint8_t> message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const SR25519Signature &signature,
        gsl::span<uint8_t> message,
        const SR25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, SR25519ProviderError)

#endif  // KAGOME_CORE_CRYPTO_SR25519_PROVIDER_HPP
