/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
        const VRFRawOutput &threshold) const = 0;

    /**
     * Compares the provided value with the threshold.
     * Required because there is a need in internal transformations of VRF
     * output to compare.
     * @param output - output of a VRF function
     * @param threshold - a value to compare against
     * @return true if the value is less than the threshold, false otherwise
     */
    virtual bool checkIfLessThanThreshold(const VRFRawOutput &output,
                                          const VRFRawOutput &threshold) const = 0;

    /**
     * Verifies that \param output was derived using \param public_key on \param
     * msg
     */
    virtual bool verify(const common::Buffer &msg,
                        const VRFOutput &output,
                        const SR25519PublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
