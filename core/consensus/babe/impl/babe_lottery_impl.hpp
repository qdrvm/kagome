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
#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus {
  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(
        std::shared_ptr<crypto::VRFProvider> vrf_provider,
        std::shared_ptr<primitives::BabeConfiguration> configuration,
        std::shared_ptr<crypto::Hasher> hasher);

    void changeEpoch(const EpochDescriptor &epoch,
                     const Randomness &randomness,
                     const Threshold &threshold,
                     const crypto::Sr25519Keypair &keypair) override;

    EpochDescriptor getEpoch() const override;

    std::optional<crypto::VRFOutput> getSlotLeadership(
        primitives::BabeSlotNumber i) const override;

    crypto::VRFOutput slotVrfSignature(
        primitives::BabeSlotNumber slot) const override;

    Randomness computeRandomness(const Randomness &last_epoch_randomness,
                                 EpochNumber last_epoch_number) override;

    void submitVRFValue(const crypto::VRFPreOutput &value) override;

    std::optional<primitives::AuthorityIndex> secondarySlotAuthor(
        primitives::BabeSlotNumber slot,
        primitives::AuthorityListSize authorities_count,
        const Randomness &randomness) override;

   private:
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    EpochLength epoch_length_;

    /// also known as "rho" (greek letter) in the spec
    std::vector<crypto::VRFPreOutput> last_epoch_vrf_values_;
    log::Logger logger_;

    EpochDescriptor epoch_;
    Randomness randomness_;
    Threshold threshold_;
    crypto::Sr25519Keypair keypair_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_IMPL_HPP
