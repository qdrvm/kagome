/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/vrf_provider.hpp"

#include <limits>
#include <optional>
#include "common/buffer.hpp"
#include "crypto/random_generator.hpp"

namespace kagome::crypto {

  class VRFProviderImpl : public VRFProvider {
    static constexpr VRFThreshold kMaxThreshold{
        std::numeric_limits<VRFThreshold>::max()};

   public:
    explicit VRFProviderImpl(std::shared_ptr<CSPRNG> generator);

    ~VRFProviderImpl() override = default;

    Sr25519Keypair generateKeypair() const override;

    std::optional<VRFOutput> sign(const common::Buffer &msg,
                                  const Sr25519Keypair &keypair,
                                  const VRFThreshold &threshold) const override;

    VRFVerifyOutput verify(const common::Buffer &msg,
                           const VRFOutput &output,
                           const Sr25519PublicKey &public_key,
                           const VRFThreshold &threshold) const override;

    std::optional<VRFOutput> signTranscript(
        const primitives::Transcript &msg,
        const Sr25519Keypair &keypair,
        const VRFThreshold &threshold) const override;

    VRFVerifyOutput verifyTranscript(
        const primitives::Transcript &msg,
        const VRFOutput &output,
        const Sr25519PublicKey &public_key,
        const VRFThreshold &threshold) const override;

    std::optional<VRFOutput> signTranscript(
        const primitives::Transcript &msg,
        const Sr25519Keypair &keypair) const override;

   private:
    std::optional<VRFOutput> signTranscriptImpl(
        const primitives::Transcript &msg,
        const Sr25519Keypair &keypair,
        const std::optional<std::reference_wrapper<const VRFThreshold>>
            threshold) const;

    std::shared_ptr<CSPRNG> generator_;
  };
}  // namespace kagome::crypto
