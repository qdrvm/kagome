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

#ifndef KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP

#include <boost/optional.hpp>

#include "common/buffer.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {

  /**
   * SR25519 based verifiable random function implementation
   */
  class VRFProvider {
   public:
    virtual ~VRFProvider() = default;

    /**
     * Generates random keypair for signing the message
     */
    virtual SR25519Keypair generateKeypair() const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     */
    virtual boost::optional<VRFOutput> sign(
        const common::Buffer &msg,
        const SR25519Keypair &keypair,
        const VRFThreshold &threshold) const = 0;

    /**
     * Verifies that \param output was derived using \param public_key on \param
     * msg
     */
    virtual VRFVerifyOutput verify(const common::Buffer &msg,
                        const VRFOutput &output,
                        const SR25519PublicKey &public_key,
                        const VRFThreshold &threshold) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
