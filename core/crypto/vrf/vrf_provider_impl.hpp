/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_VRF_VRF_HPP
#define KAGOME_CORE_CONSENSUS_VRF_VRF_HPP

#include "crypto/vrf_provider.hpp"

#include <boost/optional.hpp>
#include "common/buffer.hpp"
#include "crypto/random_generator.hpp"

namespace kagome::crypto {

  class VRFProviderImpl : public VRFProvider {
   public:
    explicit VRFProviderImpl(std::shared_ptr<CSPRNG> generator);

    ~VRFProviderImpl() override = default;

    SR25519Keypair generateKeypair() const override;

    boost::optional<VRFOutput> sign(
        const common::Buffer &msg,
        const SR25519Keypair &keypair,
        const VRFThreshold &threshold) const override;

    VRFVerifyOutput verify(const common::Buffer &msg,
                           const VRFOutput &output,
                           const SR25519PublicKey &public_key,
                           const VRFThreshold &threshold) const override;

   private:
    std::shared_ptr<CSPRNG> generator_;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CONSENSUS_VRF_VRF_HPP
