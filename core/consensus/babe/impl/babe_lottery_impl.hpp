/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_LOTTERY_IMPL_HPP
#define KAGOME_BABE_LOTTERY_IMPL_HPP

#include <memory>
#include <vector>

#include "consensus/babe/babe_lottery.hpp"
#include "crypto/hasher.hpp"
#include "crypto/vrf_provider.hpp"

namespace kagome::consensus {
  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(std::shared_ptr<crypto::VRFProvider> vrf_provider,
                    std::shared_ptr<crypto::Hasher> hasher);

    std::vector<boost::optional<crypto::VRFOutput>> slotsLeadership(
        const Epoch &epoch, crypto::SR25519Keypair keypair) const override;

    Randomness computeRandomness(Randomness last_epoch_randomness,
                                 EpochIndex last_epoch_index) override;

    void submitVRFValue(const crypto::VRFValue &value) override;

   private:
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;

    /// also known as "rho" (greek letter) in the spec
    std::vector<crypto::VRFValue> last_epoch_vrf_values_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_IMPL_HPP
