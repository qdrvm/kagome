/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP

#include <optional>

#include "common/buffer.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/transcript.hpp"

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
    virtual Sr25519Keypair generateKeypair() const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     */
    virtual std::optional<VRFOutput> sign(
        const common::Buffer &msg,
        const Sr25519Keypair &keypair,
        const VRFThreshold &threshold) const = 0;

    /**
     * Verifies that \param output was derived using \param public_key on \param
     * msg
     */
    virtual VRFVerifyOutput verify(const common::Buffer &msg,
                                   const VRFOutput &output,
                                   const Sr25519PublicKey &public_key,
                                   const VRFThreshold &threshold) const = 0;

    /**
     * Sign transcript message \param msg using \param keypair. If computed
     * value is less than \param threshold then return optional containing this
     * value and proof. Otherwise none returned
     */
    virtual std::optional<VRFOutput> signTranscript(
        const primitives::Transcript &msg,
        const Sr25519Keypair &keypair,
        const VRFThreshold &threshold) const = 0;

    /**
     * Sign transcript message \param msg using \param keypair without any
     * threshold check. Returns proof if no error happened.
     */
    virtual std::optional<VRFOutput> signTranscript(
        const primitives::Transcript &msg,
        const Sr25519Keypair &keypair) const = 0;

    /**
     * Verifies that \param output was derived using \param public_key on
     * transcript \param msg
     */
    virtual VRFVerifyOutput verifyTranscript(
        const primitives::Transcript &msg,
        const VRFOutput &output,
        const Sr25519PublicKey &public_key,
        const VRFThreshold &threshold) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
